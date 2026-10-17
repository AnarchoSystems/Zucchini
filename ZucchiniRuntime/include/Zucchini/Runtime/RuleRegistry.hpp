#pragma once

#include "Zucchini/Runtime/Rule.hpp"
#include <functional>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace nZucchini {
// Erase the scenario runner, rather than the rule object. Within each runner
// the concrete rule type is known, including throughout its step loop.
template <typename StepContext> class RuleRegistry {
public:
  template <typename ConcreteRule, typename... Args> void add(Args &&...args) {
    static_assert(std::is_base_of_v<Rule<StepContext>, ConcreteRule>,
                  "Registered rules must use this registry's StepContext");
    static_assert(
        (std::is_copy_constructible_v<std::decay_t<Args>> && ...),
        "Rule constructor arguments must be reusable for each scenario");
    auto arguments = std::make_tuple(std::forward<Args>(args)...);
    auto declaration = std::apply(
        [](const auto &...values) {
          ConcreteRule rule(values...);
          return rule.declaration();
        },
        arguments);
    const auto name = declaration.at("name").template get<std::string>();
    if (name.empty() || names.count(name)) {
      throw std::invalid_argument("Rule names must be nonempty and unique");
    }
    Entry entry;
    entry.declaration = std::move(declaration);
    entry.run = [arguments = std::move(arguments)](
                    const Zucchini &scenario, Diagnostics &diagnostics,
                    nlohmann::json *events,
                    std::optional<std::size_t> scenarioId,
                    JustifiedFacts *justified) {
      std::apply(
          [&](const auto &...values) {
            ConcreteRule rule(values...);
            Reporter reporter(diagnostics, events, scenario, scenarioId,
                              justified);
            decltype(auto) typedReporter = rule.make_reporter(reporter);
            const typename StepContext::Base context(scenario);
            rule.ConcreteRule::on_scenario_begin(context, typedReporter);
            for (std::size_t index = 0; index < scenario.steps.size();
                 ++index) {
              rule.ConcreteRule::validate(StepContext(scenario, index),
                                          typedReporter);
            }
            rule.ConcreteRule::on_scenario_end(context, typedReporter);
          },
          arguments);
    };
    entries.push_back(std::move(entry));
    names.insert(name);
  }

  nlohmann::json declarations() const {
    auto result = nlohmann::json::array();
    for (const auto &entry : entries) {
      result.push_back(entry.declaration);
    }
    return result;
  }

  // Coverage the rules report is recorded in justified, when given.
  void validate(const Zucchini &scenario, Diagnostics &diagnostics,
                nlohmann::json *ruleReport = nullptr,
                std::optional<std::size_t> scenarioId = std::nullopt,
                JustifiedFacts *justified = nullptr) const {
    for (std::size_t index = 0; index < entries.size(); ++index) {
      entries[index].run(scenario, diagnostics,
                         ruleReport ? &(*ruleReport)["rules"][index]["events"]
                                    : nullptr,
                         scenarioId, justified);
    }
  }
  std::size_t size() const { return entries.size(); }

private:
  struct Entry {
    nlohmann::json declaration;
    std::function<void(const Zucchini &, Diagnostics &, nlohmann::json *,
                       std::optional<std::size_t>, JustifiedFacts *)>
        run;
  };
  std::vector<Entry> entries;
  std::set<std::string> names;
};
} // namespace nZucchini
