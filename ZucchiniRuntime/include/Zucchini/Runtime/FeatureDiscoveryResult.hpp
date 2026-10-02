#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/PickleScenario.hpp"

#include <string>
#include <vector>

namespace nZucchini {
struct FeatureDiscoveryResult {
  std::vector<PickleScenario> pickles;
  std::string undefinedStepSuggestions;
  Diagnostics errors;
};
} // namespace nZucchini