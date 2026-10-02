#pragma once

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/StepDefinitions.hpp>

#include "Zucchini/Runtime/FeatureDiscovery.hpp"
#include "Snippets.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <cucumber/messages/pickle.hpp>

#include <string>
#include <vector>

namespace nZucchini {
struct Scenario {
  Zucchini zucchini;
  cucumber::messages::pickle pickle;
};

struct FeatureParseResult {
  std::vector<Scenario> scenarios;
  // Steps no definition matches, unique and in the order they were first seen.
  std::vector<UndefinedStep> undefinedSteps;
};

// Test adapter that links one in-memory document into zucchinis. Production
// discovery uses discover_feature_files and converts plans separately.
bool parse_feature(const std::string &source, const std::string &uri,
                   const StepDefinitions &definition,
                   FeatureParseResult &result,
                   Diagnostics &errors);
} // namespace nZucchini
