#pragma once

#include "Zucchini/Runtime/FeatureDiscoveryResult.hpp"
#include "Zucchini/Runtime/NameCasing.hpp"
#include "Zucchini/StepDefinitions.hpp"

#include <string>

namespace nZucchini {
FeatureDiscoveryResult discover_feature_files(
    const std::string &directory, const StepDefinitions &definition,
    NameCasing methodsCasing = NameCasing::SnakeCase,
    NameCasing typesCasing = NameCasing::PascalCase,
    NameCasing variablesCasing = NameCasing::CamelCase);
} // namespace nZucchini