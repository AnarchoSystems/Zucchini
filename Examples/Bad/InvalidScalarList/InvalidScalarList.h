#pragma once
#include "IInvalidScalarList.h"
namespace nInvalidScalarList {
class InvalidScalarList : public IInvalidScalarList {
public:
  void values(const std::vector<Values>&) override {}
  void jsonValues(const Values&) override {}
};
}
