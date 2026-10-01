#pragma once

#include "Zucchini/Runtime/ScenarioContext.hpp"
#include "Zucchini/Runtime/StepView.hpp"

#include <cstddef>
#include <optional>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &)>
struct StepContext : ScenarioContext<Method, ResolveMethod> {
  using Base = ScenarioContext<Method, ResolveMethod>;

  StepContext(const Zucchini &zucchini, std::size_t index)
      : Base(zucchini), index(index) {}

  std::size_t index = 0;

  StepView<Method, ResolveMethod> current() const {
    return StepView<Method, ResolveMethod>(this->zucchini.steps.at(index));
  }

  std::optional<StepView<Method, ResolveMethod>> previous() const {
    if (index == 0) {
      return std::nullopt;
    }
    return StepView<Method, ResolveMethod>(this->zucchini.steps.at(index - 1));
  }

  std::optional<StepView<Method, ResolveMethod>> next() const {
    if (index + 1 >= this->zucchini.steps.size()) {
      return std::nullopt;
    }
    return StepView<Method, ResolveMethod>(this->zucchini.steps.at(index + 1));
  }
};
} // namespace nZucchini