#include <Zucchini/Runtime/AssertedFact.hpp>
#include <Zucchini/Runtime/AssertionJustification.hpp>
#include <Zucchini/Runtime/RuleReport.hpp>
#include <Zucchini/SchemaValidator.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

namespace {
class RuleReportTest : public testing::Test {
protected:
  void SetUp() override {
    directory =
        std::filesystem::temp_directory_path() /
        ("zucchini-rule-report-" + std::to_string(std::random_device{}()));
  }
  void TearDown() override { std::filesystem::remove_all(directory); }
  nlohmann::json declaration() {
    return {{"fixture", "Fixture"},
            {"schema_version", nZucchini::rule_report_schema_version()},
            {"rules", nlohmann::json::array()},
            {"scenarios",
             {{{"test_name", "Scenario"}, {"id", 0}, {"accepted", true}}}}};
  }
  nlohmann::json result(std::string name, std::string status) {
    return {
        {"fixture", "Fixture"},        {"test_name", std::move(name)},
        {"status", std::move(status)}, {"discovery_id", "generation"},
        {"duration_ms", 125},          {"messages", nlohmann::json::array()}};
  }
  nlohmann::json read_json(const std::filesystem::path &path) {
    std::ifstream stream(directory / path);
    return nlohmann::json::parse(stream);
  }
  std::string read(const std::filesystem::path &path) {
    std::ifstream stream(directory / path);
    return {std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
  }
  std::filesystem::path directory;
};
} // namespace

TEST_F(RuleReportTest, CreatesDiscoveryDashboardWithoutExecutionResults) {
  nZucchini::store_rule_report(directory.string(), declaration());
  const auto report = read_json("rules.json");
  EXPECT_TRUE(std::filesystem::exists(directory / "index.html"));
  EXPECT_TRUE(std::filesystem::exists(directory / "dashboard.js"));
  EXPECT_NE(read("index.html").find("<script src=\"dashboard.js\"></script>"),
            std::string::npos);
  EXPECT_EQ(read("index.html").find("<script>"), std::string::npos);
  EXPECT_NE(read("dashboard.js").find("window.zucchiniRules"),
            std::string::npos);
  EXPECT_NE(read("rules.js").find("window.zucchiniRules("), std::string::npos);
  EXPECT_FALSE(report["discovery_id"].get<std::string>().empty());
  const auto key = report["scenarios"][0]["result_key"].get<std::string>();
  EXPECT_EQ(key, nZucchini::rule_result_key("Fixture", "Scenario"));
  EXPECT_FALSE(std::filesystem::exists(directory / "results"));
  EXPECT_FALSE(std::filesystem::exists(directory / "results" / (key + ".xml")));
}

TEST_F(
    RuleReportTest,
    UnchangedRediscoveryPreservesGenerationAndChangedInputsMakeResultsStale) {
  nZucchini::store_rule_report(directory.string(), declaration());
  const auto generation =
      nZucchini::rule_report_discovery_id(directory.string());
  auto data = result("Scenario", "passed");
  data["discovery_id"] = generation;
  nZucchini::store_rule_test_result(directory.string(), data);
  nZucchini::store_rule_report(directory.string(), declaration());
  EXPECT_EQ(generation,
            nZucchini::rule_report_discovery_id(directory.string()));
  auto changed = declaration();
  changed["build_id"] = "new build";
  nZucchini::store_rule_report(directory.string(), changed);
  EXPECT_NE(generation,
            nZucchini::rule_report_discovery_id(directory.string()));
  const auto key = nZucchini::rule_result_key("Fixture", "Scenario");
  EXPECT_EQ(read_json("results/" + key + ".json")["discovery_id"], generation);
}

TEST_F(RuleReportTest, ExportsEscapedJUnitAndTracksUnfinishedAttempts) {
  auto data = result("A / B: \"<test>\"", "failed");
  data["messages"].push_back({{"file", "a&b.cpp"},
                              {"line", 8},
                              {"message", "expected <value> & got \"bad\""}});
  nZucchini::store_rule_test_result(directory.string(), data);
  const auto key = nZucchini::rule_result_key("Fixture", data["test_name"]);
  const auto xml = read("results/" + key + ".xml");
  EXPECT_NE(xml.find("&lt;test&gt;"), std::string::npos);
  EXPECT_NE(xml.find("a&amp;b.cpp:8"), std::string::npos);
  EXPECT_NE(xml.find("<failure>"), std::string::npos);
  EXPECT_NE(xml.find("time=\"0.125\""), std::string::npos);
  data["status"] = "started";
  nZucchini::store_rule_test_result(directory.string(), data);
  EXPECT_FALSE(std::filesystem::exists(directory / "results" / (key + ".xml")));
  EXPECT_EQ(read_json("results/" + key + ".json")["status"], "started");
  data["status"] = "skipped";
  nZucchini::store_rule_test_result(directory.string(), data);
  EXPECT_NE(read("results/" + key + ".xml").find("<skipped>"),
            std::string::npos);
}

TEST_F(RuleReportTest, ParallelResultsDoNotOverwriteOtherScenarios) {
  auto first = result("First", "passed");
  auto second = result("Second", "failed");
  std::exception_ptr firstError;
  std::exception_ptr secondError;
  std::thread firstWriter([&] {
    try {
      for (int index = 0; index < 20; ++index) {
        nZucchini::store_rule_test_result(directory.string(), first);
      }
    } catch (...) {
      firstError = std::current_exception();
    }
  });
  std::thread secondWriter([&] {
    try {
      for (int index = 0; index < 20; ++index) {
        nZucchini::store_rule_test_result(directory.string(), second);
      }
    } catch (...) {
      secondError = std::current_exception();
    }
  });
  firstWriter.join();
  secondWriter.join();
  ASSERT_FALSE(firstError);
  ASSERT_FALSE(secondError);
  for (const auto &data : {first, second}) {
    const auto key = nZucchini::rule_result_key("Fixture", data["test_name"]);
    EXPECT_EQ(read_json("results/" + key + ".json"), data);
  }
  for (const auto &entry :
       std::filesystem::directory_iterator(directory / "results")) {
    EXPECT_NE(entry.path().filename().string().rfind(".tmp-", 0), 0);
  }
}

TEST_F(RuleReportTest, ExecutablesMayReuseFixtureNamesInSeparateDirectories) {
  auto first = declaration();
  first["executable"] = "First.exe";
  auto second = declaration();
  second["executable"] = "Second.exe";
  nZucchini::store_rule_report(directory.string(), first);
  EXPECT_THROW(nZucchini::store_rule_report(directory.string(), second),
               std::invalid_argument);
  EXPECT_NO_THROW(nZucchini::store_rule_report((directory / "second").string(), second));
  EXPECT_EQ(read_json("rules.json")["executable"], "First.exe");
}

TEST_F(RuleReportTest, AReportDirectoryCannotSilentlyReplaceAnotherFixture) {
  nZucchini::store_rule_report(directory.string(), declaration());
  auto other = declaration();
  other["fixture"] = "Other fixture";
  EXPECT_THROW(nZucchini::store_rule_report(directory.string(), other),
               std::invalid_argument);
  EXPECT_EQ(read_json("rules.json")["fixture"], "Fixture");
}

TEST_F(RuleReportTest,
       LocationsAreRelativeToFeaturesAndSnapshotsTravelWithReports) {
  const auto file = directory / "features" / "order #%.feature";
  std::filesystem::create_directories(file.parent_path());
  std::ofstream(file, std::ios::binary)
      << "Feature: Order\r\n  Scenario: Buying\r\n";
  auto data = declaration();
  data["scenarios"][0]["uri"] = file.string();
  data["scenarios"][0]["line"] = 2;
  data["feature_directory"] = (directory / "features").string();
  data["rules"].push_back(
      {{"name", "Rule"},
       {"kind", "business"},
       {"description", "Source location example"},
       {"requirements", nlohmann::json::array()},
       {"events", {{{"uri", file.string()}, {"line", 2}}}}});
  nZucchini::store_rule_report(directory.string(), data);
  const auto report = read_json("rules.json");
  EXPECT_EQ(report["sources"][file.string()],
            nlohmann::json::array({"Feature: Order", "  Scenario: Buying"}));
  std::ofstream(file, std::ios::app) << "    Given new setup\n";
  nZucchini::store_rule_report(directory.string(), data);
  EXPECT_NE(report["discovery_id"], read_json("rules.json")["discovery_id"]);
  EXPECT_EQ(report["scenarios"][0]["feature_relative_path"],
            "order #%.feature");
  EXPECT_EQ(report["rules"][0]["events"][0]["feature_relative_path"],
            "order #%.feature");
  auto execution = result("Scenario", "failed");
  execution["messages"].push_back(
      {{"file", file.string()},
       {"line", 2},
       {"message", "Failure"},
       {"gherkin_location", {{"uri", file.string()}, {"line", 2}}}});
  nZucchini::store_rule_test_result(directory.string(), execution);
  const auto key = nZucchini::rule_result_key("Fixture", "Scenario");
  const auto recorded = read_json("results/" + key + ".json");
  EXPECT_EQ(recorded["messages"][0]["feature_relative_path"],
            "order #%.feature");
  EXPECT_EQ(
      recorded["messages"][0]["gherkin_location"]["feature_relative_path"],
      "order #%.feature");
}

TEST_F(RuleReportTest,
       EmittedVersionAndSchemaReferenceComeFromVersionedSchema) {
  auto data = declaration();
  // The writer owns these fields, including for failure reports built
  // elsewhere.
  data["schema_version"] = -1;
  data["$schema"] = "incorrect";
  nZucchini::store_rule_report(directory.string(), data);
  const auto report = read_json("rules.json");
  std::ifstream stream(ZUCCHINI_RULE_REPORT_SCHEMA_PATH);
  const auto schema = nlohmann::json::parse(stream);
  EXPECT_EQ(report["schema_version"],
            schema["properties"]["schema_version"]["const"]);
  EXPECT_EQ(report["schema_version"], nZucchini::rule_report_schema_version());
  EXPECT_EQ(report["$schema"], schema["$id"]);
  EXPECT_EQ(report["$schema"], nZucchini::rule_report_schema_uri());
}

TEST_F(RuleReportTest,
       VersionedSchemaValidatesStoredReportsAndRejectsBrokenContracts) {
  const auto feature = directory / "features" / "Shop.feature";
  std::filesystem::create_directories(feature.parent_path());
  std::ofstream(feature) << "Feature: Shop\n  Scenario: Buy\n    Then debit\n";
  auto data = declaration();
  data["feature_directory"] = feature.parent_path().string();
  data["build_id"] = "build";
  data["discovery_success"] = true;
  data["discovery_diagnostics"] = nlohmann::json::array();
  auto &scenario = data["scenarios"][0];
  scenario["name"] = "Buy";
  scenario["feature"] = "Shop";
  scenario["gherkin_rule"] = "";
  scenario["uri"] = feature.string();
  scenario["line"] = 2;
  scenario["tags"] = nlohmann::json::array();
  scenario["plan_fingerprint"] = "fingerprint";
  scenario["steps"] = {{{"regex", "^debit$"},
                        {"methodName", "debit"},
                        {"text", "debit"},
                        {"line", 3},
                        {"column", 5},
                        {"captures", nlohmann::json::array()},
                        {"argument", nullptr}}};
  const nlohmann::json event = {{"kind", "coverage"},
                                {"tag", "Debit"},
                                {"message", "Wallet checked"},
                                {"scenario", "Buy"},
                                {"feature", "Shop"},
                                {"gherkin_rule", ""},
                                {"scenario_line", 2},
                                {"scenario_tags", nlohmann::json::array()},
                                {"uri", feature.string()},
                                {"line", 3},
                                {"column", 5},
                                {"step", "debit"},
                                {"scenario_id", 0},
                                {"test_name", "Scenario"},
                                {"scenario_accepted", true}};
  data["rules"] = {
      {{"name", "Wallet"},
       {"description", "Conserve credits"},
       {"kind", "business"},
       {"requirements",
        {{{"tag", "Debit"}, {"value", 0}, {"description", "Exact debit"}},
         {{"tag", "Refund"}, {"value", 1}, {"description", "Unwitnessed"}}}},
       {"events", {event}}}};
  nZucchini::store_rule_report(directory.string(), data);
  const auto report = read_json("rules.json");
  std::ifstream stream(ZUCCHINI_RULE_REPORT_SCHEMA_PATH);
  const auto schema = nlohmann::json::parse(stream);
  auto valid = [&](const nlohmann::json &document) {
    nZucchini::Diagnostics errors;
    return nZucchini::validate_document_against_schema(document, schema,
                                                       errors);
  };
  EXPECT_TRUE(valid(report));
  auto changed = report;
  changed.erase("schema_version");
  EXPECT_FALSE(valid(changed));
  changed = report;
  changed["schema_version"] = "3.0.0";
  EXPECT_FALSE(valid(changed));
  changed = report;
  changed["scenarios"][0]["accepted"] = "yes";
  EXPECT_FALSE(valid(changed));
  changed = report;
  changed["rules"][0]["events"][0]["kind"] = "violation";
  EXPECT_FALSE(valid(changed));
  changed["rules"][0]["events"][0]["severity"] = "warning";
  EXPECT_TRUE(valid(changed));
  changed = report;
  changed["future_additive_field"] = true;
  changed.erase("$schema"); // The schema version is authoritative.
  EXPECT_TRUE(valid(changed));
  changed = report;
  changed["discovery_success"] = false;
  changed["discovery_diagnostics"] = {
      {{"message", "Discovery failed"}, {"severity", "error"}}};
  changed["scenarios"] = nlohmann::json::array();
  changed["rules"][0]["events"] = nlohmann::json::array();
  EXPECT_TRUE(valid(changed));
  changed = report;
  nZucchini::AssertionJustification summary;
  summary.enforced = true;
  changed["assertion_justification"] = summary;
  changed["scenarios"][0]["asserted_facts"] = 2;
  changed["scenarios"][0]["unjustified_facts"] = {
      nZucchini::AssertedFact{0, 1, "note", 4, 9},
      nZucchini::AssertedFact{1, std::nullopt, {}, 5, 5}};
  changed["rules"][0]["events"][0]["cell"] = {
      {"row", 1}, {"key", "price"}, {"line", 4}, {"column", 7}};
  EXPECT_TRUE(valid(changed));
  changed["assertion_justification"]["severity"] = "fatal";
  EXPECT_FALSE(valid(changed));
  changed["assertion_justification"]["severity"] = "error";
  changed["assertion_justification"].erase("max_unjustified_facts_per_scenario");
  EXPECT_FALSE(valid(changed));
  changed["assertion_justification"] = nZucchini::AssertionJustification{};
  EXPECT_TRUE(valid(changed));
  changed["rules"][0]["events"][0]["cell"].erase("row");
  EXPECT_FALSE(valid(changed));
}
