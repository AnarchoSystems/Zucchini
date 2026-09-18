#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/StepDefManifest.hpp"
#include "Zucchini/Zucchini.hpp"

#include <cucumber/messages/pickle.hpp>

#include <string>
#include <vector>

namespace nZucchini
{
    struct Scenario
    {
        Zucchini zucchini;
        cucumber::messages::pickle pickle;
    };

    struct FeatureParseResult
    {
        std::vector<Scenario> scenarios;
        // Step texts no definition matches, unique and in the order they were first seen.
        std::vector<std::string> undefinedSteps;
    };

    // Compiles a Gherkin document into zucchinis, resolving feature/rule names and step locations
    // from the AST (pickles carry neither).
    bool parse_feature(const std::string& source,
                       const std::string& uri,
                       const StepDefManifest& manifest,
                       FeatureParseResult& result,
                       Diagnostics& errors);

    bool parse_feature_file(const std::string& path,
                            const StepDefManifest& manifest,
                            FeatureParseResult& result,
                            Diagnostics& errors);

    // Parses every *.feature below the directory, in sorted order.
    bool parse_feature_dir(const std::string& directory,
                           const StepDefManifest& manifest,
                           FeatureParseResult& result,
                           Diagnostics& errors);
}
