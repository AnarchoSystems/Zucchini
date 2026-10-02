#pragma once

#include "Zucchini/Runtime/StepTypeDescriptor.hpp"
#include "Zucchini/Runtime/StepTagDescriptor.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <stdexcept>
#include <type_traits>

namespace nZucchini {
template <typename Method, Method (*ResolveMethod)(const ZucchiniStep &),
          typename Tags = void,
          Tags (*ResolveTags)(const ZucchiniStep &) = nullptr>
struct ScenarioContext {
  explicit ScenarioContext(const Zucchini &zucchini) : zucchini(zucchini) {}

  Method method(const ZucchiniStep &step) const { return ResolveMethod(step); }

  template <typename TagSet = Tags,
            std::enable_if_t<!std::is_void_v<TagSet>, int> = 0>
  TagSet tags(const ZucchiniStep &step) const {
    return ResolveTags(step);
  }

  template <auto Tag>
  auto cast(Method method) const
      -> typename StepTagDescriptor<Method, Tag>::StepsType {
    return StepTagDescriptor<Method, Tag>::cast(method);
  }

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