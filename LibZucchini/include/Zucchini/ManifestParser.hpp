#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/StepDefManifest.hpp"

#include <string>

namespace nZucchini
{
    bool parse_step_def_manifest(const std::string &yaml, StepDefManifest &manifest, Diagnostics &errors);
    bool parse_step_def_manifest_from_file(const std::string &filePath, StepDefManifest &manifest, Diagnostics &errors);
}
