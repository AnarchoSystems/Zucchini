#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/Discovery.hpp"
#include "Zucchini/Runtime/FeatureDiscoveryResult.hpp"
#include "Zucchini/Runtime/NameCasing.hpp"
#include "Zucchini/Runtime/SourceLocation.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"
#include "Zucchini/StepDefinitions.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <vector>

// Test-framework-independent steps of a generated test executable. The
// generated sources own the test framework: they decide when to discover,
// build fixture-specific plans and register the resulting tests.
namespace nZucchini {
// What the generator embeds about one fixture.
struct FixtureDescription {
  std::string name;
  std::string stepDefinitionsJson;
  // Manifest locations of step definitions, keyed by diagnostic path.
  std::string stepDefinitionLocationsJson;
  NameCasing methodsCasing = NameCasing::SnakeCase;
  NameCasing typesCasing = NameCasing::PascalCase;
  NameCasing variablesCasing = NameCasing::CamelCase;
};

// Builds and validates the fixture's plans; ruleReport is null when no report
// was requested, and justification is null when asserted facts are neither
// reported nor enforced.
using MakeExecutionPlans = std::function<std::vector<Zucchini>(
    const FeatureDiscoveryResult &discovery, const StepDefinitions &definitions,
    Diagnostics &planErrors, Diagnostics &scenarioDiagnostics,
    nlohmann::json *ruleReport, AssertionJustification *justification)>;

// Discovers the feature files, stores valid execution plans and writes the rule
// coverage report. Undefined steps are merged into the manifest when its path is
// known. Diagnostics are printed to stderr; returns false when discovery failed.
bool discover_execution_plans(const DiscoveryArgs &args,
                              const FixtureDescription &fixture,
                              const std::string &executablePath,
                              const nlohmann::json &ruleDeclarations,
                              const MakeExecutionPlans &makePlans,
                              std::vector<std::string> &planNames);

// Plan names stored by an earlier discovery. Errors are printed to stderr.
bool stored_execution_plan_names(const std::string &manifestDirectory,
                                 std::vector<std::string> &planNames);

struct PlanRegistration {
  std::string name;
  // Empty uri when the plan's Gherkin source is unknown.
  SourceLocation location;
};

std::vector<PlanRegistration>
locate_execution_plans(const std::string &manifestDirectory,
                       const std::vector<std::string> &planNames);
} // namespace nZucchini
