#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/StepDefManifest.hpp"
#include "Zucchini/Zucchini.hpp"

#include <string>
#include <vector>

namespace nZucchini
{
    // Compiles a Gherkin document into zucchinis, resolving feature/rule names and step locations
    // from the AST (pickles carry neither).
    bool parse_feature(const std::string& source,
                       const std::string& uri,
                       const StepDefManifest& manifest,
                       std::vector<Zucchini>& zucchinis,
                       Diagnostics& errors);

    bool parse_feature_file(const std::string& path,
                            const StepDefManifest& manifest,
                            std::vector<Zucchini>& zucchinis,
                            Diagnostics& errors);

    // Parses every *.feature below the directory, in sorted order.
    bool parse_feature_dir(const std::string& directory,
                           const StepDefManifest& manifest,
                           std::vector<Zucchini>& zucchinis,
                           Diagnostics& errors);
}
