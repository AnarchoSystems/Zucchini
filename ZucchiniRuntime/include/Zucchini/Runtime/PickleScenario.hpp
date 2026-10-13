#pragma once

#include "Zucchini/Runtime/SourceLocation.hpp"

#include <cucumber/messages/pickle.hpp>

#include <string>
#include <vector>

namespace nZucchini {
struct PickleScenario {
  cucumber::messages::pickle pickle;
  std::string featureName;
  std::string ruleName;
  std::string uri;
  std::uint32_t line = 0;
  std::vector<SourceLocation> stepLocations;
};
} // namespace nZucchini