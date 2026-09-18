#pragma once

#include "Zucchini/StepDefManifest.hpp"
#include "Zucchini/Zucchini.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

namespace nZucchini
{
    // Passed by gtest_discover_tests as "feature_dir=<dir>" / "manifest_dir=<dir>".
    struct DiscoveryArgs
    {
        std::string featureDir;
        std::string manifestDir;
    };

    DiscoveryArgs parse_discovery_args(int argc, char** argv);

    // Parses the features, writes one manifest per zucchini and returns them. Throws on failure.
    std::vector<Zucchini> discover_zucchinis(const DiscoveryArgs& args, const StepDefManifest& manifest);

    // Reads back what discovery wrote. Throws when a manifest is missing.
    std::vector<Zucchini> load_discovered_zucchinis(const DiscoveryArgs& args);

    // Discovers when feature_dir is set, otherwise loads the stored manifests.
    void install_zucchini_provider(int argc, char** argv, StepDefManifest manifest);
    void set_zucchini_provider(std::function<std::vector<Zucchini>()> provider);

    // Evaluated lazily, i.e. after main has seen the discovery arguments.
    testing::internal::ParamGenerator<Zucchini> zucchini_values();

    std::string zucchini_test_name(const testing::TestParamInfo<Zucchini>& info);
}
