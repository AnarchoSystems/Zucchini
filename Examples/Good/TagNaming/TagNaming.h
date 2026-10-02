#pragma once

#include "ITagNaming.h"

namespace nTagNaming {
class TagNaming : public ITagNamingDefaultThrowing {
public:
  void taggedStep() override {}
};
} // namespace nTagNaming