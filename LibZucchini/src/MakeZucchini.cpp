#include "Zucchini/MakeZucchini.hpp"

namespace nZucchini
{
    bool make_zucchini(const cucumber::messages::pickle& pickle,
                       const StepDefManifest& manifest,
                       const std::string& featureName,
                       Zucchini& zucchini,
                       Diagnostics& errors)
    {
        // TODO: implement; the declaration and the tests define the contract.
        (void)pickle;
        (void)manifest;
        (void)featureName;
        zucchini = Zucchini{};
        errors.clear();
        return false;
    }
}
