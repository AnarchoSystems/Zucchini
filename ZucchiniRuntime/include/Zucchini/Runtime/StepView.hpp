#pragma once

#include "Zucchini/Runtime/Zucchini.hpp"

namespace nZucchini {
template <typename Method> struct StepView {
  Method method;
  const ZucchiniStep &step;
};
} // namespace nZucchini