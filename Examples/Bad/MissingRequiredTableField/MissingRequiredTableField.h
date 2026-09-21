#pragma once

#include "IMissingRequiredTableField.h"

namespace nMissingRequiredTableField {
class MissingRequiredTableField : public IMissingRequiredTableField {
public:
  void addEntries(const std::vector<Entry> &) override {}
};
} // namespace nMissingRequiredTableField