#include "Zucchini/Runtime/PlanDiscovery.hpp"

#include "Zucchini/Runtime/ExecutionPlan.hpp"
#include "Zucchini/Runtime/FeatureDiscovery.hpp"
#include "Zucchini/Runtime/ManifestUpdate.hpp"
#include "Zucchini/Runtime/RuleReport.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>

namespace nZucchini {
namespace {
void report_diagnostics(const std::string &prefix,
                        const Diagnostics &diagnostics) {
  if (!diagnostics.empty()) {
    std::cerr << prefix << to_string(diagnostics) << std::endl;
  }
}

const char *severity_name(DiagnosticSeverity severity) {
  switch (severity) {
  case DiagnosticSeverity::Error:
    return "error";
  case DiagnosticSeverity::Warning:
    return "warning";
  case DiagnosticSeverity::Info:
    return "info";
  }
  return "info";
}

bool write_rule_report(const DiscoveryArgs &args, nlohmann::json report,
                       const Diagnostics &diagnostics,
                       const std::string &fixture, const std::string &buildId,
                       const std::string &executable) {
  if (args.ruleCoverageReport.empty()) {
    return true;
  }
  try {
    report["build_id"] = buildId;
    report["executable"] = executable;
    report["feature_directory"] = args.featureDir;
    report["fixture"] = fixture;
    report["discovery_diagnostics"] = nlohmann::json::array();
    for (const auto &diagnostic : diagnostics) {
      report["discovery_diagnostics"].push_back(
          {{"message", to_string(diagnostic)},
           {"severity", severity_name(diagnostic.severity)}});
    }
    report["discovery_success"] = !has_errors(diagnostics);
    store_rule_report(args.ruleCoverageReport, std::move(report));
    return true;
  } catch (const std::exception &error) {
    std::cerr << "cannot write rule coverage report: " << error.what()
              << std::endl;
    return false;
  }
}

// Manifest warnings point at the generated definitions until relocated to the
// step definition's source in the manifest.
void relocate_definition_warnings(Diagnostics &warnings,
                                  const nlohmann::json &locations,
                                  const std::string &stepDefinitionsPath) {
  for (auto &warning : warnings) {
    const auto location = locations.find(warning.path);
    if (location != locations.end()) {
      warning.path = location->at("file").get<std::string>();
      warning.line = location->at("line").get<std::uint32_t>();
      warning.column = location->at("column").get<std::uint32_t>();
    } else if (!stepDefinitionsPath.empty()) {
      warning.path = stepDefinitionsPath;
    }
  }
}

void report_undefined_steps(const DiscoveryArgs &args,
                            const std::string &suggestions) {
  if (!args.stepDefinitionsPath.empty()) {
    std::string updateError;
    if (update_step_definitions(args.stepDefinitionsPath, suggestions,
                                updateError)) {
      std::cerr << "undefined step definitions. please review "
                << step_definitions_url(args.stepDefinitionsPath) << std::endl;
      return;
    }
    std::cerr << "could not update step definitions: " << updateError
              << std::endl;
  }
  std::cerr << "undefined steps in '" << args.featureDir
            << "'; add these step definitions:\n\n"
            << suggestions << std::endl;
}
} // namespace

bool discover_execution_plans(const DiscoveryArgs &args,
                              const FixtureDescription &fixture,
                              const std::string &executablePath,
                              const nlohmann::json &ruleDeclarations,
                              const MakeExecutionPlans &makePlans,
                              std::vector<std::string> &planNames) {
  const auto buildId = args.ruleCoverageReport.empty()
                           ? std::string()
                           : rule_report_build_id(executablePath);
  const auto executable =
      std::filesystem::path(executablePath).filename().string();
  const auto definitions = nlohmann::json::parse(fixture.stepDefinitionsJson)
                               .get<StepDefinitions>();
  auto discovery = discover_feature_files(
      args.featureDir, definitions, fixture.methodsCasing, fixture.typesCasing,
      fixture.variablesCasing);
  const auto parseErrorPrefix =
      "cannot parse features in '" + args.featureDir + "':";
  report_diagnostics(parseErrorPrefix, discovery.errors);
  if (has_errors(discovery.errors) ||
      !discovery.undefinedStepSuggestions.empty()) {
    auto diagnostics = discovery.errors;
    if (!discovery.undefinedStepSuggestions.empty()) {
      add_diagnostic(diagnostics, args.featureDir,
                     "Undefined step definitions prevent discovery");
    }
    const nlohmann::json report = {
        {"schema_version", rule_report_schema_version()},
        {"rules", ruleDeclarations},
        {"scenarios", nlohmann::json::array()}};
    if (!write_rule_report(args, report, diagnostics, fixture.name, buildId,
                           executable)) {
      return false;
    }
  }
  if (has_errors(discovery.errors)) {
    return false;
  }
  relocate_definition_warnings(
      discovery.warnings,
      nlohmann::json::parse(fixture.stepDefinitionLocationsJson),
      args.stepDefinitionsPath);
  report_diagnostics("discovery diagnostics:", discovery.warnings);
  if (!discovery.undefinedStepSuggestions.empty()) {
    report_undefined_steps(args, discovery.undefinedStepSuggestions);
    return false;
  }

  Diagnostics planErrors;
  Diagnostics scenarioDiagnostics;
  nlohmann::json ruleReport;
  // Reports record asserted facts even when justification is not enforced.
  auto justification = args.justification;
  const bool report = !args.ruleCoverageReport.empty();
  const auto plans = makePlans(
      discovery, definitions, planErrors, scenarioDiagnostics,
      report ? &ruleReport : nullptr,
      report || justification.enforced ? &justification : nullptr);
  if (report && planErrors.empty()) {
    ruleReport["assertion_justification"] = justification;
  }
  auto reportDiagnostics = planErrors;
  reportDiagnostics.insert(reportDiagnostics.end(),
                           scenarioDiagnostics.begin(),
                           scenarioDiagnostics.end());
  if (!write_rule_report(args, std::move(ruleReport), reportDiagnostics,
                         fixture.name, buildId, executable)) {
    return false;
  }
  report_diagnostics(parseErrorPrefix, planErrors);
  report_diagnostics("scenario diagnostics:", scenarioDiagnostics);
  if (has_errors(planErrors) || has_errors(scenarioDiagnostics)) {
    return false;
  }

  Diagnostics storeErrors;
  if (!store_execution_plans(args.manifestDir, plans, storeErrors)) {
    report_diagnostics("cannot store execution plans:", storeErrors);
    return false;
  }
  for (const auto &plan : plans) {
    planNames.push_back(execution_plan_name(plan));
  }
  return true;
}

bool stored_execution_plan_names(const std::string &manifestDirectory,
                                 std::vector<std::string> &planNames) {
  Diagnostics errors;
  if (!list_execution_plan_names(manifestDirectory, planNames, errors)) {
    report_diagnostics("cannot list execution plans:", errors);
    return false;
  }
  return true;
}

std::vector<PlanRegistration>
locate_execution_plans(const std::string &manifestDirectory,
                       const std::vector<std::string> &planNames) {
  const auto locations = execution_plan_locations(manifestDirectory);
  std::vector<PlanRegistration> registrations;
  registrations.reserve(planNames.size());
  for (const auto &name : planNames) {
    Zucchini plan;
    const auto location = locations.find(name);
    if (location != locations.end()) {
      plan.uri = location->second.uri;
      plan.line = location->second.line;
    }
    // Without an index entry, the stored plan supplies its own location.
    Diagnostics errors;
    const bool known =
        (location != locations.end() ||
         load_execution_plan(manifestDirectory, name, plan, errors)) &&
        !plan.uri.empty() && plan.line != 0;
    registrations.push_back(
        {name, known ? SourceLocation(plan.uri, plan.line, 0) : SourceLocation()});
  }
  return registrations;
}
} // namespace nZucchini
