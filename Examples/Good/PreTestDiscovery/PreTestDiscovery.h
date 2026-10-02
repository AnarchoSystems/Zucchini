#pragma once

#include "IPreTestDiscovery.h"

#include <stdexcept>

namespace nPreTestDiscovery {
class PreTestDiscovery : public IPreTestDiscovery {
public:
  void valueIs(long value) override {
    if (value != 42) {
      throw std::runtime_error("unexpected value");
    }
  }
};
} // namespace nPreTestDiscovery
