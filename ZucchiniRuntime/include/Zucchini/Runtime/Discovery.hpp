#pragma once

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/StepDefinitions.hpp>

#include "Zucchini/Runtime/NameCasing.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <cucumber/messages/pickle.hpp>
#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

namespace nZucchini {
// Installed by the generated code; runs the fixture's validate_scenario hook
// during discovery.
using ScenarioValidator = std::function<void(
    const Zucchini &, const cucumber::messages::pickle &, Diagnostics &)>;

// Passed by gtest_discover_tests as "feature_dir=<dir>" / "manifest_dir=<dir>".
struct DiscoveryArgs {
  std::string featureDir;
  std::string manifestDir;
};

DiscoveryArgs parse_discovery_args(int argc, char **argv);

// Parses the features, reports diagnostics, writes one manifest per zucchini
// and returns them. Discovery fails when any reported diagnostic has error
// severity. `methodsCasing`/`classesCasing` control the casing of
// undefined-step snippets suggested for the user (from the stylesheet).
std::vector<Zucchini>
discover_zucchinis(const DiscoveryArgs &args,
                   const StepDefinitions &definition,
                   const ScenarioValidator &validate = {},
                   NameCasing methodsCasing = NameCasing::SnakeCase,
                   NameCasing classesCasing = NameCasing::PascalCase);

// Reads back what discovery wrote. Throws when a manifest is missing.
std::vector<Zucchini> load_discovered_zucchinis(const DiscoveryArgs &args);

// Discovers when feature_dir is set, otherwise loads the stored manifests.
void install_zucchini_provider(
    int argc, char **argv, StepDefinitions definition,
    ScenarioValidator validate = {},
    NameCasing snippetMethodsCasing = NameCasing::SnakeCase,
    NameCasing snippetClassesCasing = NameCasing::PascalCase);
void set_zucchini_provider(std::function<std::vector<Zucchini>()> provider);

// Evaluated lazily, i.e. after main has seen the discovery arguments.
testing::internal::ParamGenerator<Zucchini> zucchini_values();

std::string zucchini_test_name(const testing::TestParamInfo<Zucchini> &info);
} // namespace nZucchini
