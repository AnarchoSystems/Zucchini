#pragma once

#include "Zucchini/Runtime/StepView.hpp"

#include <cstddef>
#include <optional>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &)>
struct StepContext {
  const Zucchini &zucchini;
  std::size_t index = 0;

  StepView<Method> current() const {
    const auto &step = zucchini.steps.at(index);
    return StepView<Method>{ResolveMethod(step), step};
  }

  std::optional<StepView<Method>> previous() const {
    if (index == 0) {
      return std::nullopt;
    }
    const auto &step = zucchini.steps.at(index - 1);
    return StepView<Method>{ResolveMethod(step), step};
  }

  std::optional<StepView<Method>> next() const {
    if (index + 1 >= zucchini.steps.size()) {
      return std::nullopt;
    }
    const auto &step = zucchini.steps.at(index + 1);
    return StepView<Method>{ResolveMethod(step), step};
  }
};
} // namespace nZucchini