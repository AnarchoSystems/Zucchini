#include <Zucchini/Runtime/RuleReport.hpp>
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
            {"schema_version", 2},
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

TEST_F(RuleReportTest, CreatesFolderDashboardAndIsolatedResultPlaceholders) {
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
  EXPECT_EQ(read_json("results/" + key + ".json")["status"], "not_run");
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
