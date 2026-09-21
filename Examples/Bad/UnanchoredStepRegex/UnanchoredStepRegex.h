#pragma once

#include "IUnanchoredStepRegex.h"

namespace nUnanchoredStepRegex {
class UnanchoredStepRegex : public IUnanchoredStepRegex {
public:
  void doNothing() override {}
};
} // namespace nUnanchoredStepRegex