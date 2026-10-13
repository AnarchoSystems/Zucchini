#pragma once
#include "Zucchini/Runtime/Reporter.hpp"
#include "Zucchini/Runtime/RuleMetadata.hpp"
namespace nZucchini {
template <typename StepContext> struct Rule {
  using ScenarioContext = typename StepContext::Base;
  virtual ~Rule() = default;
  virtual RuleMetadata metadata() const = 0;
  Reporter &make_reporter(Reporter &reporter) const { return reporter; }
  virtual void on_scenario_begin(const ScenarioContext &, Reporter &) {}
  virtual void validate(const StepContext &, Reporter &) = 0;
  virtual void on_scenario_end(const ScenarioContext &, Reporter &) {}
  virtual nlohmann::json declaration() const = 0;
};
} // namespace nZucchini
