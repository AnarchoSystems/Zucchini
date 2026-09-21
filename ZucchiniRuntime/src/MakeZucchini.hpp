#pragma once

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/StepDefinitions.hpp>

#include "Zucchini/Runtime/Zucchini.hpp"

#include <cucumber/messages/pickle.hpp>
#include <string>

namespace nZucchini {
// Returns the first step definition whose regex matches the text, or nullptr.
const StepDefinition *find_step_definition(
    const StepDefinitions &definition, const std::string &text);

// The feature name is passed in because a pickle does not carry it.
bool make_zucchini(const cucumber::messages::pickle &pickle,
                   const StepDefinitions &definition,
                   const std::string &featureName, Zucchini &zucchini,
                   Diagnostics &errors);
} // namespace nZucchini
