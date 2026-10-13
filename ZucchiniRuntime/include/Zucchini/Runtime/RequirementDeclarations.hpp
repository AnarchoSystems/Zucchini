#pragma once

#include "Zucchini/Runtime/RuleMetadata.hpp"
#include <utility>
#include <vector>

namespace nZucchini {
// Declaration order is report order; enum values remain the keys used by
// reporters.
template <typename Requirement>
using RequirementDeclarations =
    std::vector<std::pair<Requirement, RequirementMetadata>>;
} // namespace nZucchini
