#pragma once

#include "Zucchini/Runtime/AssertedFact.hpp"
#include "Zucchini/Runtime/AssertionJustification.hpp"
#include "Zucchini/Runtime/ExecutionPlan.hpp"
#include "Zucchini/Runtime/FeatureDiscoveryResult.hpp"
#include "Zucchini/Runtime/ScenarioContext.hpp"
#include "Zucchini/Runtime/SourceLocation.hpp"
#include "Zucchini/Runtime/StepContext.hpp"

#include "Zucchini/Runtime/RuleRegistry.hpp"
#include "Zucchini/Runtime/RuleReport.hpp"
#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &),
          typename Tags = void,
          Tags (*ResolveTags)(const ZucchiniStep &) = nullptr>
class ScenarioFixture {
public:
  using ScenarioContextType =
      ScenarioContext<Method, ResolveMethod, Tags, ResolveTags>;
  using StepContextType = StepContext<Method, ResolveMethod, Tags, ResolveTags>;

  virtual ~ScenarioFixture() = default;

  virtual void SetUp() {}
  virtual void TearDown() {}

  template <typename ValidateArguments>
  std::vector<Zucchini> make_execution_plans(
      const FeatureDiscoveryResult &discovery,
      const StepDefinitions &definition, ValidateArguments &&validateArguments,
      Diagnostics &planErrors, Diagnostics &scenarioDiagnostics,
      nlohmann::json *ruleReport = nullptr,
      const RuleRegistry<StepContextType> &registry = {},
      // Records asserted facts when given; see AssertionJustification.
      AssertionJustification *justification = nullptr) {
    if (ruleReport) {
      *ruleReport = {{"schema_version", nZucchini::rule_report_schema_version()},
                     {"rules", registry.declarations()},
                     {"scenarios", nlohmann::json::array()}};
    }
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
    std::size_t scenarioId = 0;
    for (auto &plan : plans) {
      std::vector<std::size_t> eventOffsets;
      if (ruleReport) {
        for (const auto &entry : (*ruleReport)["rules"]) {
          eventOffsets.push_back(entry["events"].size());
        }
      }
      Diagnostics errors;
      validateArguments(plan, errors);
      JustifiedFacts justified;
      registry.validate(plan, errors, ruleReport, scenarioId,
                        justification ? &justified : nullptr);
      std::vector<AssertedFact> facts;
      std::vector<AssertedFact> unjustified;
      if (justification) {
        facts = asserted_facts(plan);
        unjustified = unjustified_facts(facts, justified);
        justification->add_scenario(facts.size(), unjustified.size());
        if (justification->enforced) {
          enforce_justification(plan, facts, unjustified, *justification,
                                errors);
        }
      }
      scenarioDiagnostics.insert(scenarioDiagnostics.end(), errors.begin(),
                                 errors.end());
      if (!has_errors(errors)) {
        const auto originalName = plan.name;
        std::size_t suffix = 1;
        while (!names.insert(execution_plan_name(plan)).second) {
          plan.name = originalName + " #" + std::to_string(++suffix);
        }
      }
      if (ruleReport) {
        (*ruleReport)["scenarios"].push_back(
            {{"id", scenarioId},
             {"name", plan.name},
             {"feature", plan.featureName},
             {"gherkin_rule", plan.ruleName},
             {"uri", plan.uri},
             {"line", plan.line},
             {"tags", plan.tags},
             {"steps", plan.steps},
             {"test_name", execution_plan_name(plan)},
             {"plan_fingerprint",
              rule_result_key("plan", nlohmann::json(plan).dump())},
             {"accepted", !has_errors(errors)}});
        if (justification) {
          auto &scenario = (*ruleReport)["scenarios"].back();
          scenario["asserted_facts"] = facts.size();
          scenario["unjustified_facts"] = unjustified;
        }
        for (std::size_t ruleIndex = 0; ruleIndex < registry.size();
             ++ruleIndex) {
          auto &events = (*ruleReport)["rules"][ruleIndex]["events"];
          for (std::size_t index = eventOffsets[ruleIndex];
               index < events.size(); ++index) {
            events[index]["test_name"] = execution_plan_name(plan);
            events[index]["scenario_accepted"] = !has_errors(errors);
          }
        }
      }
      if (!has_errors(errors)) {
        validPlans.push_back(std::move(plan));
      }
      ++scenarioId;
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