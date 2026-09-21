#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/StepDefManifest.hpp"
#include "Zucchini/Zucchini.hpp"

#include <cucumber/messages/pickle.hpp>
#include <string>

namespace nZucchini {
// Returns the first step definition whose regex matches the text, or nullptr.
const StepDef *find_step_def(const StepDefManifest &manifest,
                             const std::string &text);

// The feature name is passed in because a pickle does not carry it.
bool make_zucchini(const cucumber::messages::pickle &pickle,
                   const StepDefManifest &manifest,
                   const std::string &featureName, Zucchini &zucchini,
                   Diagnostics &errors);
} // namespace nZucchini
