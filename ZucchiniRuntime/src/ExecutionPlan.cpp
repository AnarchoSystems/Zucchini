#include "Zucchini/Runtime/ExecutionPlan.hpp"

#include "MakeZucchini.hpp"
#include "ManifestStore.hpp"
#include "Naming.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>

namespace nZucchini {
namespace {
bool step_index_from_path(const std::string &path, std::size_t &index,
                          std::string &detail) {
  constexpr char prefix[] = "steps[";
  if (path.rfind(prefix, 0) != 0) {
    return false;
  }
  const auto close = path.find(']', sizeof(prefix) - 1);
  if (close == std::string::npos) {
    return false;
  }
  try {
    index = static_cast<std::size_t>(std::stoull(
        path.substr(sizeof(prefix) - 1, close - (sizeof(prefix) - 1))));
  } catch (const std::exception &) {
    return false;
  }
  detail = close + 1 < path.size() && path[close + 1] == '.'
               ? path.substr(close + 2)
               : std::string();
  return true;
}
} // namespace

bool make_execution_plan(const PickleScenario &scenario,
                         const StepDefinitions &definition, Zucchini &plan,
                         Diagnostics &errors) {
  Diagnostics conversionErrors;
  if (!make_zucchini(scenario.pickle, definition, scenario.featureName, plan,
                     conversionErrors)) {
    for (auto &error : conversionErrors) {
      std::size_t stepIndex = 0;
      std::string detail;
      if (step_index_from_path(error.path, stepIndex, detail) &&
          stepIndex < scenario.stepLocations.size()) {
        const auto &location = scenario.stepLocations[stepIndex];
        error.path = location.uri;
        error.line = location.line;
        error.column = location.column;
        if (!detail.empty()) {
          error.message = detail + ": " + error.message;
        }
      } else if (!scenario.uri.empty()) {
        error.path = scenario.uri +
                     (error.path.empty() ? std::string() : "." + error.path);
      }
      errors.push_back(std::move(error));
    }
    return false;
  }

  plan.ruleName = scenario.ruleName;
  plan.uri = scenario.uri;
  plan.line = scenario.line;
  for (std::size_t index = 0;
       index < plan.steps.size() && index < scenario.stepLocations.size();
       ++index) {
    plan.steps[index].line = scenario.stepLocations[index].line;
    plan.steps[index].column = scenario.stepLocations[index].column;
  }
  return true;
}

std::string execution_plan_name(const Zucchini &plan) {
  return test_name(plan);
}

bool store_execution_plans(const std::string &directory,
                           const std::vector<Zucchini> &plans,
                           Diagnostics &errors) {
  if (!store_zucchinis(directory, plans, errors)) return false;
  const auto path = std::filesystem::path(directory) / "locations.index";
  try {
    nlohmann::json locations = nlohmann::json::object();
    for (const auto &plan : plans)
      locations[execution_plan_name(plan)] = {{"uri", plan.uri}, {"line", plan.line}};
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << locations.dump() << '\n';
    stream.close();
    if (!stream) throw std::runtime_error("cannot write execution plan locations");
  } catch (const std::exception &error) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    add_diagnostic(errors, path.string(), error.what());
    return false;
  }
  return true;
}

bool load_execution_plans(const std::string &directory,
                          std::vector<Zucchini> &plans,
                          Diagnostics &errors) {
  return load_zucchinis(directory, plans, errors);
}

bool list_execution_plan_names(const std::string &directory,
                               std::vector<std::string> &names,
                               Diagnostics &errors) {
  return list_manifest_names(directory, names, errors);
}

std::map<std::string, SourceLocation>
execution_plan_locations(const std::string &directory) {
  std::map<std::string, SourceLocation> locations;
  std::ifstream stream(std::filesystem::path(directory) / "locations.index");
  if (!stream) return locations;
  try {
    const auto index = nlohmann::json::parse(stream);
    for (const auto &entry : index.items())
      locations.emplace(entry.key(), SourceLocation(
          entry.value().at("uri").get<std::string>(),
          entry.value().at("line").get<std::uint32_t>(), 0, ""));
  } catch (const nlohmann::json::exception &) {
    locations.clear(); // Older or interrupted discovery: use individual plans.
  }
  return locations;
}

bool load_execution_plan(const std::string &directory,
                         const std::string &testName, Zucchini &plan,
                         Diagnostics &errors) {
  return load_zucchini(directory, testName, plan, errors);
}
} // namespace nZucchini