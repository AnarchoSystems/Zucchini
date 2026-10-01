#pragma once

#include "Zucchini/Runtime/StepTypeDescriptor.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <stdexcept>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &)>
struct ScenarioContext {
  explicit ScenarioContext(const Zucchini &zucchini) : zucchini(zucchini) {}

  Method method(const ZucchiniStep &step) const { return ResolveMethod(step); }

  template <Method Value>
  auto getArgs(const ZucchiniStep &step) const
      -> typename StepTypeDescriptor<Method, Value>::ArgsType {
    if (ResolveMethod(step) != Value) {
      throw std::logic_error("requested step arguments for a different method");
    }
    using ArgsType = typename StepTypeDescriptor<Method, Value>::ArgsType;
    return ArgsType(step);
  }

  const Zucchini &zucchini;
};
} // namespace nZucchini