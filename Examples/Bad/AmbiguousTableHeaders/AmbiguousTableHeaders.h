#pragma once

#include "IAmbiguousTableHeaders.h"

namespace nAmbiguousTableHeaders {
class AmbiguousTableHeaders : public IAmbiguousTableHeaders {
public:
  void addEntries(const std::vector<Entry> &) override {}
};
} // namespace nAmbiguousTableHeaders