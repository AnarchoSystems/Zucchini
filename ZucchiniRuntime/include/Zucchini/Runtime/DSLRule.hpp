#pragma once
#include "Zucchini/Runtime/Rule.hpp"
#include "Zucchini/Runtime/ViolationReporter.hpp"
#include <set>
namespace nZucchini {
template <typename StepContext, typename Requirement>
struct DSLRule : Rule<StepContext> {
  using ScenarioContext = typename Rule<StepContext>::ScenarioContext;
  static_assert(std::is_enum_v<Requirement>, "Requirement must be an enum");
  using Requirements = RequirementDeclarations<Requirement>;
  virtual Requirements requirements() const = 0;
  ViolationReporter<Requirement> make_reporter(Reporter &reporter) const {
    return ViolationReporter<Requirement>(reporter, requirements());
  }
  virtual void on_scenario_begin(const ScenarioContext &,
                                 ViolationReporter<Requirement> &) {}
  void on_scenario_begin(const ScenarioContext &context,
                         Reporter &reporter) final {
    auto violations = make_reporter(reporter);
    on_scenario_begin(context, violations);
  }
  virtual void on_scenario_end(const ScenarioContext &,
                               ViolationReporter<Requirement> &) {}
  void on_scenario_end(const ScenarioContext &context,
                       Reporter &reporter) final {
    auto violations = make_reporter(reporter);
    on_scenario_end(context, violations);
  }
  virtual void validate(const StepContext &,
                        ViolationReporter<Requirement> &) = 0;
  void validate(const StepContext &context, Reporter &reporter) final {
    auto violations = make_reporter(reporter);
    validate(context, violations);
  }
  nlohmann::json declaration() const final {
    const auto info = this->metadata();
    nlohmann::json result = {{"name", info.name},
                             {"description", info.description},
                             {"kind", "dsl"},
                             {"requirements", nlohmann::json::array()},
                             {"events", nlohmann::json::array()}};
    std::set<std::string> names;
    std::set<Requirement> values;
    for (const auto &[value, metadata] : requirements()) {
      if (!values.insert(value).second) {
        throw std::invalid_argument(
            "Each requirement enum value may be declared only once");
      }
      if (metadata.name.empty() || !names.insert(metadata.name).second) {
        throw std::invalid_argument(
            "Requirement names must be nonempty and unique");
      }
      result["requirements"].push_back(
          {{"tag", metadata.name},
           {"value", static_cast<std::underlying_type_t<Requirement>>(value)},
           {"description", metadata.description}});
    }
    return result;
  }
};
} // namespace nZucchini
