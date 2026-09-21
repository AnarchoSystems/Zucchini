#pragma once

#include "IInvalidEnumValue.h"

namespace nInvalidEnumValue {
class InvalidEnumValue : public IInvalidEnumValue {
public:
  void addTaggedEntries(const std::vector<TaggedEntry> &) override {}
};
} // namespace nInvalidEnumValue