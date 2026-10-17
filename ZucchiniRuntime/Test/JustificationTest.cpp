#include <Zucchini/Runtime/ZucchiniRuntime.hpp>
#include <gtest/gtest.h>

namespace {
using namespace nZucchini;

ZucchiniStep outcome(std::string text, StepArgument argument = {},
                     std::uint32_t line = 0) {
  ZucchiniStep step("", "", std::move(text), {}, std::move(argument), line, 5);
  step.keywordType = StepKeywordType::Outcome;
  return step;
}

DataTableArgument keyed_table() {
  return DataTableArgument(
      {{{"item", "apple"}, {"total", "6"}, {"note", "crunchy"}},
       {{"item", "pear"}, {"total", "5"}, {"note", ""}}},
      {{{"item", {{"line", 4}, {"column", 9}}},
        {"total", {{"line", 4}, {"column", 17}}},
        {"note", {{"line", 4}, {"column", 23}}}},
       {{"item", {{"line", 5}, {"column", 9}}},
        {"total", {{"line", 5}, {"column", 17}}},
        {"note", {{"line", 5}, {"column", 23}}}}});
}
} // namespace

TEST(TableCell, ResolvesAliasesPositionsAndAbsentCells) {
  const auto keyed = outcome("basket", keyed_table());
  EXPECT_EQ(TableCell(1, {"subtotal", "total"}, 1).key(keyed), "total");
  EXPECT_EQ(TableCell(0, std::string("item")).key(keyed), "item");
  EXPECT_EQ(TableCell(0, {"quantity"}, 3).key(keyed), std::nullopt);
  EXPECT_EQ(TableCell(0, std::size_t{0}).key(keyed), std::nullopt);
  EXPECT_THROW(TableCell(2, std::string("item")).key(keyed),
               std::out_of_range);

  const auto positional =
      outcome("positional", DataTableArgument({{"apple", "6"}}));
  EXPECT_EQ(TableCell(0, {"total"}, 1).key(positional), "1");
  EXPECT_EQ(TableCell(0, std::size_t{2}).key(positional), std::nullopt);
  EXPECT_EQ(TableCell(0, std::string("total")).key(positional), std::nullopt);
  EXPECT_THROW(TableCell(0, std::size_t{0}).key(outcome("no table")),
               std::out_of_range);
}

TEST(Justification, ReporterRecordsCellsStepsAndCopiedSteps) {
  Zucchini scenario("scenario", "feature",
                    {outcome("basket", keyed_table(), 3), outcome("total")});
  scenario.uri = "basket.feature";
  Diagnostics diagnostics;
  auto events = nlohmann::json::array();
  JustifiedFacts justified;
  Reporter reporter(diagnostics, &events, scenario, std::nullopt, &justified);
  reporter.coverage("Subtotal", "checked", scenario.steps[0],
                    TableCell(0, {"subtotal", "total"}, 1));
  const auto copy = scenario.steps[1];
  reporter.coverage("Total", "checked", copy);
  // Absent cells and scenario-level coverage justify nothing.
  reporter.coverage("Quantity", "absent", scenario.steps[0],
                    TableCell(0, std::string("quantity")));
  reporter.coverage("Scenario", "whole scenario");
  reporter.violation("Item", DiagnosticSeverity::Warning, "unexpected",
                     scenario.steps[0], TableCell(1, std::string("item")));

  EXPECT_TRUE(justified.justifies(0, 0, "total"));
  EXPECT_FALSE(justified.justifies(0, 1, "total"));
  EXPECT_FALSE(justified.justifies(0));
  EXPECT_TRUE(justified.justifies(1));
  EXPECT_FALSE(justified.justifies(1, 0, "quantity"));
  EXPECT_FALSE(justified.justifies(0, 1, "item"));

  ASSERT_EQ(events.size(), 5);
  EXPECT_EQ(events[0]["cell"], (nlohmann::json{{"row", 0},
                                               {"key", "total"},
                                               {"line", 4},
                                               {"column", 17}}));
  EXPECT_FALSE(events[1].contains("cell"));
  EXPECT_EQ(events[2]["cell"], (nlohmann::json{{"row", 0}}));
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics[0].line, 5);
  EXPECT_EQ(diagnostics[0].column, 9);
}

TEST(Justification, AssertsOutcomeStepsAndCellsOutsideIgnoredColumns) {
  auto context = outcome("given");
  context.keywordType = StepKeywordType::Context;
  auto unknown = outcome("asterisk");
  unknown.keywordType = StepKeywordType::Unknown;
  auto ignoring = keyed_table();
  ignoring.ignoredKeys = {"item"};
  const Zucchini scenario(
      "scenario", "feature",
      {context, unknown, outcome("basket", keyed_table(), 3),
       outcome("document", DocStringArgument("{}")),
       outcome("header only", DataTableArgument(std::vector<nlohmann::json>{}), 9),
       outcome("ignoring", ignoring, 11)});
  const auto facts = asserted_facts(scenario);
  ASSERT_EQ(facts.size(), 12);
  EXPECT_EQ(facts[0].step, 2);
  EXPECT_EQ(*facts[0].row, 0);
  EXPECT_EQ(facts[0].key, "item");
  EXPECT_EQ(facts[0].line, 4);
  EXPECT_EQ(facts[0].column, 9);
  // The pear's empty note may assert absence.
  EXPECT_EQ(facts[4].key, "note");
  EXPECT_EQ(facts[4].line, 5);
  EXPECT_EQ(facts[6].step, 3);
  EXPECT_FALSE(facts[6].row);
  EXPECT_EQ(facts[7].step, 4);
  EXPECT_FALSE(facts[7].row);
  EXPECT_EQ(facts[7].line, 9);
  for (std::size_t index = 8; index < facts.size(); ++index) {
    EXPECT_EQ(facts[index].step, 5);
    EXPECT_NE(facts[index].key, "item");
  }
}

TEST(Justification, ReportsOneDiagnosticPerStepWithListedCells) {
  std::vector<nlohmann::json> rows(3, {{"a", "1"}, {"b", "2"}});
  const Zucchini scenario(
      "scenario", "feature",
      {outcome("keyed", DataTableArgument(rows), 3),
       outcome("positional", DataTableArgument({{"x", "y"}}), 7),
       outcome("plain", {}, 9)},
      "", "basket.feature");
  Diagnostics diagnostics;
  report_unjustified_facts(scenario,
                           unjustified_facts(asserted_facts(scenario), {}),
                           DiagnosticSeverity::Info, diagnostics);
  ASSERT_EQ(diagnostics.size(), 3);
  EXPECT_EQ(diagnostics[0].message,
            "no business rule justifies 6 asserted table cells: 'a' in row 1, "
            "'b' in row 1, 'a' in row 2, 'b' in row 2, 'a' in row 3 and 1 "
            "more");
  EXPECT_EQ(diagnostics[0].line, 3);
  EXPECT_EQ(diagnostics[0].severity, DiagnosticSeverity::Info);
  EXPECT_EQ(diagnostics[1].message,
            "no business rule justifies 2 asserted table cells: column 0 in "
            "row 1, column 1 in row 1");
  EXPECT_EQ(diagnostics[2].message, "no business rule justifies this assertion");
  EXPECT_EQ(diagnostics[2].path, "basket.feature");
}

namespace {
enum class Method { Step };
Method resolve(const ZucchiniStep &) { return Method::Step; }
using Context = StepContext<Method, resolve>;
enum class Requirement { Checked };

// Justifies every step whose text says so.
struct Justifying final : BusinessRule<Context, Requirement> {
  RuleMetadata metadata() const override { return {"justifying", ""}; }
  Requirements requirements() const override {
    return {{Requirement::Checked, {"Checked", "A justified step"}}};
  }
  void validate(const Context &context,
                BusinessReporter<Requirement> &reporter) override {
    if (context.current().step.text == "justified") {
      reporter.coverage(Requirement::Checked, "checked",
                        context.current().step);
    }
  }
};

PickleScenario pickle(std::string name, std::vector<std::string> outcomes) {
  PickleScenario result;
  result.pickle.name = std::move(name);
  result.featureName = "Feature";
  result.uri = "justification.feature";
  result.line = 2;
  for (auto &text : outcomes) {
    cucumber::messages::pickle_step step;
    step.text = std::move(text);
    step.type = cucumber::messages::pickle_step_type::OUTCOME;
    result.pickle.steps.push_back(std::move(step));
  }
  return result;
}
} // namespace

TEST(Justification, FixtureRejectsUnjustifiedAssertionsAndTotalsFacts) {
  RuleRegistry<Context> registry;
  registry.add<Justifying>();
  FeatureDiscoveryResult discovery;
  discovery.pickles = {pickle("Justified", {"justified"}),
                       pickle("Incidental", {"justified", "incidental"})};
  const StepDefinitions definitions(
      {StepDefinition("^justified$", "justified"),
       StepDefinition("^incidental$", "incidental")});
  ScenarioFixture<Method, resolve> fixture;
  Diagnostics planErrors;
  Diagnostics diagnostics;
  nlohmann::json report;
  AssertionJustification justification;
  justification.enforced = true;
  justification.severity = DiagnosticSeverity::Error;
  const auto plans = fixture.make_execution_plans(
      discovery, definitions, [](const auto &, auto &) {}, planErrors,
      diagnostics, &report, registry, &justification);
  EXPECT_TRUE(planErrors.empty());
  ASSERT_EQ(plans.size(), 1);
  EXPECT_EQ(plans[0].name, "Justified");
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics[0].severity, DiagnosticSeverity::Error);
  EXPECT_EQ(justification.scenarios, 2);
  EXPECT_EQ(justification.assertedFacts, 3);
  EXPECT_EQ(justification.unjustifiedFacts, 1);
  EXPECT_EQ(justification.scenariosOverLimit, 0);
  EXPECT_EQ(report["scenarios"][0]["asserted_facts"], 1);
  EXPECT_TRUE(report["scenarios"][0]["unjustified_facts"].empty());
  EXPECT_EQ(report["scenarios"][1]["unjustified_facts"][0]["step_index"], 1);
  EXPECT_EQ(report["scenarios"][1]["accepted"], false);

  // Unenforced, the same scenarios are accepted and their facts still
  // reported.
  Diagnostics unchecked;
  nlohmann::json plainReport;
  AssertionJustification recorded;
  EXPECT_EQ(fixture
                .make_execution_plans(discovery, definitions,
                                      [](const auto &, auto &) {}, planErrors,
                                      unchecked, &plainReport, registry,
                                      &recorded)
                .size(),
            2);
  EXPECT_TRUE(unchecked.empty());
  EXPECT_EQ(plainReport["scenarios"][1]["unjustified_facts"].size(), 1);
  EXPECT_EQ(recorded.unjustifiedFacts, 1);
}

TEST(Justification, EnforcementRejectsScenariosWithoutAssertionsOrOverLimit) {
  RuleRegistry<Context> registry;
  registry.add<Justifying>();
  FeatureDiscoveryResult discovery;
  auto silent = pickle("Silent", {"justified"});
  silent.pickle.steps[0].type = cucumber::messages::pickle_step_type::ACTION;
  discovery.pickles = {silent,
                       pickle("Incidental", {"incidental", "incidental"}),
                       pickle("Tolerated", {"incidental"})};
  const StepDefinitions definitions(
      {StepDefinition("^justified$", "justified"),
       StepDefinition("^incidental$", "incidental")});
  ScenarioFixture<Method, resolve> fixture;
  Diagnostics planErrors;
  Diagnostics diagnostics;
  AssertionJustification justification;
  justification.enforced = true;
  justification.severity = DiagnosticSeverity::Info;
  justification.maxUnjustifiedFactsPerScenario = 1;
  const auto plans = fixture.make_execution_plans(
      discovery, definitions, [](const auto &, auto &) {}, planErrors,
      diagnostics, nullptr, registry, &justification);
  ASSERT_EQ(plans.size(), 1);
  EXPECT_EQ(plans[0].name, "Tolerated");
  std::vector<std::string> errors;
  for (const auto &diagnostic : diagnostics) {
    if (diagnostic.severity == DiagnosticSeverity::Error) {
      errors.push_back(diagnostic.message);
    }
  }
  EXPECT_EQ(errors,
            (std::vector<std::string>{
                "scenario asserts nothing; add an outcome step such as Then",
                "scenario asserts 2 facts that no business rule justifies; at "
                "most 1 are allowed"}));
  EXPECT_EQ(justification.scenariosWithoutAssertions, 1);
  EXPECT_EQ(justification.scenariosOverLimit, 1);
}

TEST(Justification, SummaryIncludesSettingsOnlyWhenEnforced) {
  AssertionJustification justification;
  justification.add_scenario(4, 3);
  justification.add_scenario(0, 0);
  nlohmann::json summary = justification;
  EXPECT_EQ(summary, (nlohmann::json{{"enforced", false},
                                     {"scenarios", 2},
                                     {"asserted_facts", 4},
                                     {"unjustified_facts", 3}}));
  justification.enforced = true;
  summary = justification;
  EXPECT_EQ(summary["severity"], "warning");
  EXPECT_EQ(summary["max_unjustified_facts_per_scenario"], 2);
  EXPECT_EQ(summary["scenarios_without_assertions"], 1);
  EXPECT_EQ(summary["scenarios_over_limit"], 1);
}

TEST(Justification, DiscoveryArgumentsEnableTheCheck) {
  const char *defaults[] = {"test", "feature_dir=features"};
  EXPECT_FALSE(parse_discovery_args(2, const_cast<char **>(defaults))
                   .justification.enforced);
  const char *argv[] = {"test", "--unjustified-assertions=error",
                        "--max-unjustified-facts-per-scenario=0"};
  const auto args = parse_discovery_args(3, const_cast<char **>(argv));
  EXPECT_TRUE(args.justification.enforced);
  EXPECT_EQ(args.justification.severity, DiagnosticSeverity::Error);
  EXPECT_EQ(args.justification.maxUnjustifiedFactsPerScenario, 0);
  const char *limitOnly[] = {"test", "--max-unjustified-facts-per-scenario=3"};
  const auto limited = parse_discovery_args(2, const_cast<char **>(limitOnly));
  EXPECT_TRUE(limited.justification.enforced);
  EXPECT_EQ(limited.justification.severity, DiagnosticSeverity::Warning);
  const char *invalid[] = {"test", "--unjustified-assertions=fatal"};
  EXPECT_THROW(parse_discovery_args(2, const_cast<char **>(invalid)),
               std::invalid_argument);
  EXPECT_THROW(parse_count("-1"), std::invalid_argument);
  EXPECT_THROW(parse_count("1.5"), std::invalid_argument);
  EXPECT_THROW(parse_count(""), std::invalid_argument);
}
