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

TEST(Justification, FixtureTotalsFactsWithoutRejectingScenarios) {
  RuleRegistry<Context> registry;
  registry.add<Justifying>();
  FeatureDiscoveryResult discovery;
  auto silent = pickle("Silent", {"justified"});
  silent.pickle.steps[0].type = cucumber::messages::pickle_step_type::ACTION;
  discovery.pickles = {pickle("Justified", {"justified"}),
                       pickle("Incidental", {"justified", "incidental"}),
                       silent};
  const StepDefinitions definitions(
      {StepDefinition("^justified$", "justified"),
       StepDefinition("^incidental$", "incidental")});
  ScenarioFixture<Method, resolve> fixture;
  Diagnostics planErrors;
  Diagnostics diagnostics;
  nlohmann::json report;
  AssertionJustification justification;
  const auto plans = fixture.make_execution_plans(
      discovery, definitions, [](const auto &, auto &) {}, planErrors,
      diagnostics, &report, registry, &justification);
  EXPECT_TRUE(planErrors.empty());
  EXPECT_TRUE(diagnostics.empty());
  EXPECT_EQ(plans.size(), 3);
  EXPECT_EQ(justification.scenarios, 3);
  EXPECT_EQ(justification.scenariosWithoutAssertions, 1);
  EXPECT_EQ(justification.assertedFacts, 3);
  EXPECT_EQ(justification.unjustifiedFacts, 1);
  EXPECT_EQ(report["scenarios"][0]["asserted_facts"], 1);
  EXPECT_TRUE(report["scenarios"][0]["unjustified_facts"].empty());
  EXPECT_EQ(report["scenarios"][1]["unjustified_facts"][0]["step_index"], 1);
  EXPECT_EQ(report["scenarios"][1]["accepted"], true);
  EXPECT_EQ(report["scenarios"][2]["asserted_facts"], 0);
}

TEST(Justification, SummaryReportsTotalsAndNeverEnforces) {
  AssertionJustification justification;
  justification.add_scenario(4, 3);
  justification.add_scenario(0, 0);
  nlohmann::json summary = justification;
  EXPECT_EQ(summary, (nlohmann::json{{"enforced", false},
                                     {"scenarios", 2},
                                     {"scenarios_without_assertions", 1},
                                     {"asserted_facts", 4},
                                     {"unjustified_facts", 3}}));
}

TEST(Justification, RequirementsDeclareStrictness) {
  struct Declaring final : BusinessRule<Context, Requirement> {
    RuleMetadata metadata() const override { return {"declaring", ""}; }
    Requirements requirements() const override {
      return {{Requirement::Checked,
               {"Checked", "A checked step", Strictness::Bounded}}};
    }
    void validate(const Context &, BusinessReporter<Requirement> &) override {}
  };
  EXPECT_EQ(Declaring().declaration()["requirements"][0]["strictness"],
            "bounded");
  EXPECT_EQ(Justifying().declaration()["requirements"][0]["strictness"],
            "unspecified");
  EXPECT_STREQ(strictness_name(Strictness::Exact), "exact");
  EXPECT_STREQ(strictness_name(Strictness::OneOf), "one_of");
  EXPECT_STREQ(strictness_name(Strictness::OpenEnded), "open_ended");
}
