#include "Zucchini/ManifestParser.hpp"

namespace nZucchini
{
    bool parse_step_def_manifest(const std::string& yaml, StepDefManifest& manifest, Diagnostics& errors)
    {
        // TODO: implement; the declaration and the tests define the contract.
        (void)yaml;
        manifest = StepDefManifest{};
        errors.clear();
        return false;
    }
}
