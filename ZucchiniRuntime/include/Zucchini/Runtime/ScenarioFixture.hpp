#pragma once

#include "Zucchini/Runtime/ExecutionPlan.hpp"
#include "Zucchini/Runtime/FeatureDiscoveryResult.hpp"
#include "Zucchini/Runtime/ScenarioContext.hpp"
#include "Zucchini/Runtime/SourceLocation.hpp"
#include "Zucchini/Runtime/StepContext.hpp"

#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &)>
class ScenarioFixture {
public:
  using ScenarioContextType = ScenarioContext<Method, ResolveMethod>;
  using StepContextType = StepContext<Method, ResolveMethod>;

  virtual ~ScenarioFixture() = default;

  virtual void set_up() {}
  virtual void tear_down() {}

  template <typename ValidateArguments, typename ValidateScenario>
  std::vector<Zucchini>
  make_execution_plans(const FeatureDiscoveryResult &discovery,
                       const StepDefinitions &definition,
                       ValidateArguments &&validateArguments,
                       ValidateScenario &&validateScenario,
                       Diagnostics &planErrors,
                       Diagnostics &scenarioDiagnostics) {
    planErrors.insert(planErrors.end(), discovery.errors.begin(),
                      discovery.errors.end());
    if (!discovery.errors.empty() ||
        !discovery.undefinedStepSuggestions.empty()) {
      return {};
    }

    std::vector<Zucchini> plans;
    plans.reserve(discovery.pickles.size());
    for (const auto &pickle : discovery.pickles) {
      Zucchini plan;
      if (!make_execution_plan(pickle, definition, plan, planErrors)) {
        continue;
      }
      plans.push_back(std::move(plan));
    }
    if (!planErrors.empty()) {
      return {};
    }

    std::vector<Zucchini> validPlans;
    std::set<std::string> names;
    validPlans.reserve(plans.size());
    for (auto &plan : plans) {
      Diagnostics errors;
      validateArguments(plan, errors);
      const ScenarioContextType context(plan);
      validateScenario(context, errors);
      scenarioDiagnostics.insert(scenarioDiagnostics.end(), errors.begin(),
                                 errors.end());
      if (!has_errors(errors)) {
        const auto originalName = plan.name;
        std::size_t suffix = 1;
        while (!names.insert(execution_plan_name(plan)).second) {
          plan.name = originalName + " #" + std::to_string(++suffix);
        }
        validPlans.push_back(std::move(plan));
      }
    }
    return validPlans;
  }

  template <typename AroundStep, typename Dispatch>
  void execute_plan(const Zucchini &plan, AroundStep &&aroundStep,
                    Dispatch &&dispatch) {
    try {
      for (std::size_t index = 0; index < plan.steps.size(); ++index) {
        const auto &step = plan.steps[index];
        set_current_source_location(
            SourceLocation(plan.uri, step.line, step.column, step.text));
        const StepContextType context(plan, index);
        aroundStep(context, [&] { dispatch(step); });
      }
    } catch (...) {
      clear_current_source_location();
      throw;
    }
    clear_current_source_location();
  }
};
} // namespace nZucchini