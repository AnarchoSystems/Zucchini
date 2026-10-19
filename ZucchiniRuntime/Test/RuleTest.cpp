#include <Zucchini/Runtime/ZucchiniRuntime.hpp>
#include <gtest/gtest.h>

namespace {
enum class Method { Step };
Method resolve(const nZucchini::ZucchiniStep &) { return Method::Step; }
using Context = nZucchini::StepContext<Method, resolve>;
enum class Requirement { Seen, Missing };

struct Business : nZucchini::BusinessRule<Context, Requirement> {
  int steps = 0;
  int scenarios = 0;
  nZucchini::RuleMetadata metadata() const override {
    return {"business", "Checks meaning"};
  }
  Requirements requirements() const override {
    return {{Requirement::Seen, {"Seen", "A step was seen"}},
            {Requirement::Missing, {"Missing", "An uncovered requirement"}}};
  }
  void on_scenario_begin(const ScenarioContext &,
                         nZucchini::BusinessReporter<Requirement> &) override {}
  void on_scenario_end(const ScenarioContext &,
                       nZucchini::BusinessReporter<Requirement> &) override {
    ++scenarios;
  }
  void validate(const Context &context,
                nZucchini::BusinessReporter<Requirement> &reporter) override {
    ++steps;
    reporter.coverage(Requirement::Seen, "Step found", context.current().step);
    reporter.violation(Requirement::Seen,
                       nZucchini::DiagnosticSeverity::Warning, "Review step",
                       context.current().step);
  }
};
struct DSL : nZucchini::DSLRule<Context, Requirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"dsl", "Checks syntax"};
  }
  Requirements requirements() const override {
    return {{Requirement::Seen,
             {"Valid sequence", "Steps must follow the protocol"}}};
  }
  void validate(const Context &context,
                nZucchini::ViolationReporter<Requirement> &reporter) override {
    reporter.violation(Requirement::Seen, nZucchini::DiagnosticSeverity::Error,
                       "Invalid sequence", context.current().step);
  }
};
} // namespace

TEST(Rules, TypedEventsAndDiagnosticsHaveSourceAndSeverity) {
  nZucchini::Zucchini scenario("scenario", "feature",
                               {{"", "", "step", {}, {}, 7, 3}}, "",
                               "file.feature");
  nZucchini::Diagnostics diagnostics;
  Business business;
  auto declaration = business.declaration();
  nZucchini::Reporter reporter(diagnostics, &declaration["events"], scenario);
  nZucchini::Rule<Context> &rule = business;
  rule.on_scenario_begin(Context::Base(scenario), reporter);
  rule.validate(Context(scenario, 0), reporter);
  rule.on_scenario_end(Context::Base(scenario), reporter);
  EXPECT_EQ(business.steps, 1);
  EXPECT_EQ(business.scenarios, 1);
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics.front().severity,
            nZucchini::DiagnosticSeverity::Warning);
  EXPECT_EQ(diagnostics.front().line, 7);
  EXPECT_FALSE(nZucchini::has_errors(diagnostics));
  ASSERT_EQ(declaration["requirements"].size(), 2);
  ASSERT_EQ(declaration["events"].size(), 2);
  EXPECT_EQ(declaration["events"][0]["tag"], "Seen");
  EXPECT_EQ(declaration["events"][0]["kind"], "coverage");
  EXPECT_EQ(declaration["events"][1]["severity"], "warning");
  EXPECT_EQ(declaration["events"][1]["uri"], "file.feature");
}

TEST(Rules, DSLViolationsAreTaggedAndReportingIsOptional) {
  nZucchini::Zucchini scenario("scenario", "feature", {{"", "", "step"}});
  nZucchini::Diagnostics diagnostics;
  DSL dsl;
  auto declaration = dsl.declaration();
  nZucchini::Reporter reporter(diagnostics, &declaration["events"], scenario);
  nZucchini::Rule<Context> &rule = dsl;
  rule.validate(Context(scenario, 0), reporter);
  ASSERT_EQ(declaration["requirements"].size(), 1);
  EXPECT_EQ(declaration["requirements"][0]["description"],
            "Steps must follow the protocol");
  EXPECT_EQ(declaration["events"][0]["tag"], "Valid sequence");
  EXPECT_TRUE(nZucchini::has_errors(diagnostics));
  nZucchini::Reporter withoutReport(diagnostics, nullptr, scenario);
  rule.validate(Context(scenario, 0), withoutReport);
  EXPECT_EQ(diagnostics.size(), 2);
}

TEST(Rules, UndeclaredRequirementsAreRejected) {
  nZucchini::Zucchini scenario;
  nZucchini::ZucchiniStep step;
  nZucchini::Diagnostics diagnostics;
  nZucchini::Reporter reporter(diagnostics, nullptr, scenario);
  const nZucchini::RequirementDeclarations<Requirement> declarations;
  nZucchini::BusinessReporter<Requirement> business(reporter, declarations);
  EXPECT_THROW(business.coverage(Requirement::Seen, "missing", step),
               std::out_of_range);
  nZucchini::ViolationReporter<Requirement> dsl(reporter, declarations);
  EXPECT_THROW(dsl.violation(Requirement::Seen,
                             nZucchini::DiagnosticSeverity::Error, "missing",
                             step),
               std::out_of_range);
  EXPECT_TRUE(diagnostics.empty());
}

TEST(Rules, DiscoveryArgumentPreservesPathsWithSpacesAndBackslashes) {
  const char *argv[] = {
      "test", "--rule-coverage-report=C:\\reports folder\\coverage.json"};
  const auto args =
      nZucchini::parse_discovery_args(2, const_cast<char **>(argv));
  EXPECT_EQ(args.ruleCoverageReport, "C:\\reports folder\\coverage.json");
  EXPECT_TRUE(args.ruleDashboard);
  EXPECT_TRUE(nZucchini::parse_discovery_args(1, const_cast<char **>(argv))
                  .ruleCoverageReport.empty());
}

TEST(Rules, DiscoveryArgumentsChooseReportDataAndDashboardStylesheet) {
  const char *argv[] = {"test", "--rule-report-data=C:\\reports folder",
                        "--rule-coverage-report-css=C:\\styles\\my report.css",
                        // Removed options are ignored.
                        "--unjustified-assertions=error"};
  const auto args =
      nZucchini::parse_discovery_args(4, const_cast<char **>(argv));
  EXPECT_EQ(args.ruleCoverageReport, "C:\\reports folder");
  EXPECT_FALSE(args.ruleDashboard);
  EXPECT_EQ(args.ruleDashboardStylesheet, "C:\\styles\\my report.css");
}

namespace {
struct Lifecycle final : nZucchini::BusinessRule<Context, Requirement> {
  Lifecycle(std::vector<int> *starts, std::vector<int> *ends, int limit)
      : starts(starts), ends(ends), limit(limit) {}
  nZucchini::RuleMetadata metadata() const override {
    return {"lifecycle", "Checks the final accumulated state"};
  }
  Requirements requirements() const override {
    return {{Requirement::Seen, {"Seen", "Scenario processed"}},
            {Requirement::Missing, {"Limit", "Step count stays within limit"}}};
  }
  void on_scenario_begin(
      const ScenarioContext &,
      nZucchini::BusinessReporter<Requirement> &reporter) override {
    starts->push_back(steps);
    reporter.coverage(Requirement::Seen, "Begin");
  }
  void validate(const Context &,
                nZucchini::BusinessReporter<Requirement> &) override {
    ++steps;
  }
  void
  on_scenario_end(const ScenarioContext &Context,
                  nZucchini::BusinessReporter<Requirement> &reporter) override {
    ends->push_back(steps);
    if (steps > limit) {
      reporter.violation(Requirement::Missing,
                         nZucchini::DiagnosticSeverity::Error,
                         "Final state exceeds the configured limit");
    } else {
      reporter.coverage(Requirement::Missing, "Final state is within limit");
    }
  }
  int steps = 0;
  std::vector<int> *starts;
  std::vector<int> *ends;
  int limit;
};

struct Silent final : nZucchini::BusinessRule<Context, Requirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"silent", "Unobserved requirements"};
  }
  Requirements requirements() const override {
    return {{Requirement::Seen, {"Seen", "Relevant invariant is tested"}}};
  }
  void validate(const Context &,
                nZucchini::BusinessReporter<Requirement> &) override {}
};

nZucchini::PickleScenario pickle(std::string name, int steps,
                                 std::uint32_t line) {
  nZucchini::PickleScenario result;
  result.pickle.name = std::move(name);
  result.featureName = "Feature";
  result.uri = "rules.feature";
  result.line = line;
  for (int index = 0; index < steps; ++index) {
    cucumber::messages::pickle_step step;
    step.text = "step";
    result.pickle.steps.push_back(std::move(step));
  }
  return result;
}
} // namespace

TEST(Rules, RegistryCreatesFreshRulesAndHooksReportAtScenarioLocations) {
  nZucchini::RuleRegistry<Context> registry;
  std::vector<int> starts;
  std::vector<int> ends;
  registry.add<Lifecycle>(&starts, &ends, 1);
  nZucchini::FeatureDiscoveryResult discovery;
  discovery.pickles = {pickle("Too many", 2, 3), pickle("Within limit", 1, 7),
                       pickle("Empty", 0, 11)};
  nZucchini::ScenarioFixture<Method, resolve> fixture;
  nZucchini::Diagnostics planErrors;
  nZucchini::Diagnostics diagnostics;
  nlohmann::json report;
  const auto plans = fixture.make_execution_plans(
      discovery,
      nZucchini::StepDefinitions({nZucchini::StepDefinition("^step$", "step")}),
      [](const auto &, auto &) {}, planErrors, diagnostics, &report, registry);
  EXPECT_TRUE(planErrors.empty());
  EXPECT_EQ(plans.size(), 2);
  EXPECT_EQ(starts, (std::vector<int>{0, 0, 0}));
  EXPECT_EQ(ends, (std::vector<int>{2, 1, 0}));
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics[0].line, 3);
  EXPECT_EQ(diagnostics[0].path, "rules.feature");
  ASSERT_EQ(report["scenarios"].size(), 3);
  EXPECT_EQ(report["scenarios"][0]["accepted"], false);
  EXPECT_EQ(report["scenarios"][1]["accepted"], true);
  const auto &events = report["rules"][0]["events"];
  ASSERT_EQ(events.size(), 6);
  EXPECT_EQ(events[0]["message"], "Begin");
  EXPECT_EQ(events[1]["kind"], "violation");
  EXPECT_EQ(events[1]["scenario_id"], report["scenarios"][0]["id"]);
  EXPECT_EQ(events[1]["scenario_accepted"], false);
  EXPECT_EQ(events[4]["line"], 11);
  EXPECT_EQ(events[5]["kind"], "coverage");
}

TEST(Rules, UncoveredRequirementsRemainDerivableWithoutEmittingFailures) {
  nZucchini::RuleRegistry<Context> registry;
  registry.add<Silent>();
  nZucchini::FeatureDiscoveryResult discovery;
  discovery.pickles = {pickle("Empty", 0, 5)};
  nZucchini::ScenarioFixture<Method, resolve> fixture;
  nZucchini::Diagnostics errors;
  nZucchini::Diagnostics diagnostics;
  nlohmann::json report;
  const auto plans = fixture.make_execution_plans(
      discovery, nZucchini::StepDefinitions{}, [](const auto &, auto &) {},
      errors, diagnostics, &report, registry);
  EXPECT_TRUE(errors.empty());
  EXPECT_TRUE(diagnostics.empty());
  ASSERT_EQ(plans.size(), 1);
  ASSERT_EQ(report["scenarios"].size(), 1);
  EXPECT_EQ(report["rules"][0]["requirements"][0]["tag"], "Seen");
  EXPECT_TRUE(report["rules"][0]["events"].empty());
}

TEST(Rules, RegistryRejectsDuplicateNamesAndKeepsExistingRegistrations) {
  nZucchini::RuleRegistry<Context> registry;
  registry.add<Business>();
  EXPECT_THROW(registry.add<Business>(), std::invalid_argument);
  registry.add<DSL>();
  EXPECT_EQ(registry.size(), 2);
  nZucchini::Diagnostics diagnostics;
  registry.validate(
      nZucchini::Zucchini("scenario", "feature", {{"", "", "step"}}),
      diagnostics);
  EXPECT_EQ(diagnostics.size(), 2);
  EXPECT_TRUE(nZucchini::has_errors(diagnostics));
}

namespace {
struct ReorderedBusiness : Business {
  Requirements requirements() const override {
    return {{Requirement::Missing, {"Declared first", "Enum value one"}},
            {Requirement::Seen, {"Declared second", "Enum value zero"}}};
  }
};
struct DuplicateRequirement : Business {
  Requirements requirements() const override {
    return {{Requirement::Seen, {"First", "First declaration"}},
            {Requirement::Seen, {"Second", "Duplicate enum value"}}};
  }
};
} // namespace

TEST(Rules, DeclarationOrderIsIndependentOfEnumValuesAndReporterLookup) {
  ReorderedBusiness business;
  auto declaration = business.declaration();
  ASSERT_EQ(declaration["requirements"].size(), 2);
  EXPECT_EQ(declaration["requirements"][0]["tag"], "Declared first");
  EXPECT_EQ(declaration["requirements"][0]["value"], 1);
  EXPECT_EQ(declaration["requirements"][1]["tag"], "Declared second");
  EXPECT_EQ(declaration["requirements"][1]["value"], 0);
  nZucchini::Zucchini scenario("scenario", "feature", {{"", "", "step"}});
  nZucchini::Diagnostics diagnostics;
  nZucchini::Reporter reporter(diagnostics, &declaration["events"], scenario);
  nZucchini::Rule<Context> &rule = business;
  rule.validate(Context(scenario, 0), reporter);
  ASSERT_EQ(declaration["events"].size(), 2);
  EXPECT_EQ(declaration["events"][0]["tag"], "Declared second");
  EXPECT_EQ(declaration["events"][1]["tag"], "Declared second");
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics[0].severity, nZucchini::DiagnosticSeverity::Warning);
}

TEST(Rules, DuplicateEnumValuesAreRejectedInsteadOfSilentlyCollapsed) {
  DuplicateRequirement business;
  EXPECT_THROW(business.declaration(), std::invalid_argument);
  nZucchini::Zucchini scenario;
  nZucchini::Diagnostics diagnostics;
  nZucchini::Reporter reporter(diagnostics, nullptr, scenario);
  EXPECT_THROW(business.make_reporter(reporter), std::invalid_argument);
}

namespace {
template <typename T, typename = void> struct HasCoverage : std::false_type {};
template <typename T>
struct HasCoverage<T, std::void_t<decltype(std::declval<T &>().coverage(
                          Requirement::Seen, std::string{}))>>
    : std::true_type {};
static_assert(!HasCoverage<nZucchini::ViolationReporter<Requirement>>::value);
static_assert(HasCoverage<nZucchini::BusinessReporter<Requirement>>::value);

struct EndOnlyDSL : DSL {
  void validate(const Context &,
                nZucchini::ViolationReporter<Requirement> &) override {}
  void on_scenario_end(
      const ScenarioContext &,
      nZucchini::ViolationReporter<Requirement> &reporter) override {
    reporter.violation(Requirement::Seen, nZucchini::DiagnosticSeverity::Error,
                       "Incomplete protocol");
  }
};
struct DuplicateDSL : DSL {
  Requirements requirements() const override {
    return {{Requirement::Seen, {"First", "First declaration"}},
            {Requirement::Seen, {"Second", "Duplicate enum value"}}};
  }
};
} // namespace

TEST(Rules, DSLDeclarationsRemainSilentUntilAnEndViolation) {
  nZucchini::Zucchini scenario("scenario", "feature", {{"", "", "step"}});
  scenario.uri = "protocol.feature";
  scenario.line = 4;
  EndOnlyDSL dsl;
  auto declaration = dsl.declaration();
  nZucchini::Diagnostics diagnostics;
  nZucchini::Reporter reporter(diagnostics, &declaration["events"], scenario);
  nZucchini::Rule<Context> &rule = dsl;
  rule.on_scenario_begin(Context::Base(scenario), reporter);
  rule.validate(Context(scenario, 0), reporter);
  EXPECT_TRUE(declaration["events"].empty());
  EXPECT_TRUE(diagnostics.empty());
  rule.on_scenario_end(Context::Base(scenario), reporter);
  ASSERT_EQ(declaration["events"].size(), 1);
  EXPECT_EQ(declaration["events"][0]["tag"], "Valid sequence");
  EXPECT_EQ(declaration["events"][0]["kind"], "violation");
  EXPECT_EQ(declaration["events"][0]["line"], 4);
  ASSERT_EQ(diagnostics.size(), 1);
  EXPECT_EQ(diagnostics[0].path, "protocol.feature");
  EXPECT_EQ(diagnostics[0].line, 4);
}

TEST(Rules, DSLRejectsDuplicateDeclaredEnumValues) {
  DuplicateDSL dsl;
  EXPECT_THROW(dsl.declaration(), std::invalid_argument);
  nZucchini::Zucchini scenario;
  nZucchini::Diagnostics diagnostics;
  nZucchini::Reporter reporter(diagnostics, nullptr, scenario);
  EXPECT_THROW(dsl.make_reporter(reporter), std::invalid_argument);
}
