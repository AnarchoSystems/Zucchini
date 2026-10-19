#pragma once
#include "Zucchini/Runtime/Strictness.hpp"

#include <string>

namespace nZucchini {
struct RuleMetadata {
  std::string name;
  std::string description;
};
struct RequirementMetadata {
  // A readable report tag; use the enum case spelling when desired.
  // C++17 cannot derive that spelling through enum reflection.
  std::string name;
  std::string description;
  // Reported for business requirements; DSL constraints ignore it.
  Strictness strictness = Strictness::Unspecified;
};
} // namespace nZucchini
