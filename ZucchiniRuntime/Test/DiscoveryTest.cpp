#include <Zucchini/Runtime/Discovery.hpp>
#include <Zucchini/Runtime/FeatureDiscovery.hpp>

#include "ManifestStore.hpp"
#include "Naming.hpp"
#include <Zucchini/Runtime/SourceLocation.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
using namespace nZucchini;

Zucchini Sample(std::string scenario, std::string rule = {}) {
  return Zucchini(std::move(scenario), "Calculator",
                  {ZucchiniStep("^I add (\\d+)$", "add", "I add 2",
                                {Capture("value", 2)}, {}, 7, 5)},
                  std::move(rule));
}

std::string TempDir(const std::string &name) {
  const auto path =
      std::filesystem::temp_directory_path() / ("zucchini-test-" + name);
  std::filesystem::remove_all(path);
  return path.string();
}

TEST(ManifestStore, RoundTripsZucchinisByTestName) {
  const auto directory = TempDir("store");
  const std::vector<Zucchini> zucchinis = {Sample("Adding", "Addition"),
                                           Sample("Subtracting")};

  Diagnostics errors;
  ASSERT_TRUE(store_zucchinis(directory, zucchinis, errors))
      << to_string(errors);
  EXPECT_TRUE(std::filesystem::exists(
      manifest_path(directory, test_name(zucchinis[0]))));

  Zucchini single;
  ASSERT_TRUE(load_zucchini(directory, test_name(zucchinis[0]), single, errors))
      << to_string(errors);
  EXPECT_EQ(zucchinis[0], single);

  std::vector<Zucchini> loaded;
  ASSERT_TRUE(load_zucchinis(directory, loaded, errors)) << to_string(errors);
  ASSERT_EQ(2u, loaded.size());

  std::filesystem::remove_all(directory);
}

TEST(ManifestStore, ReportsMissingManifests) {
  const auto directory = TempDir("missing");

  Zucchini zucchini;
  Diagnostics errors;
  EXPECT_FALSE(load_zucchini(directory, "Nope", zucchini, errors));
  ASSERT_FALSE(errors.empty());
  EXPECT_NE(std::string::npos,
            errors.front().message.find("re-run test discovery"));
}

TEST(Discovery, ParsesDiscoveryArguments) {
  const char *argv[] = {"test", "--gtest_list_tests", "feature_dir=/features",
                        "manifest_dir=/manifests"};

  const auto args = parse_discovery_args(4, const_cast<char **>(argv));

  EXPECT_EQ("/features", args.featureDir);
  EXPECT_EQ("/manifests", args.manifestDir);
}

TEST(Discovery, ProducesRawPicklesWithoutBuildingPlans) {
  const auto directory = TempDir("pickles");
  std::filesystem::create_directories(directory);
  {
    std::ofstream feature(std::filesystem::path(directory) / "feature.feature");
    feature << "Feature: Calculator\n\n"
               "  Scenario: Adding\n"
               "    Given I add 2\n";
  }
  const StepDefinitions definition(
      {StepDefinition("^I add (\\d+)$", "add",
                      {Argument("value", "int")})});

  const auto discovery = discover_feature_files(directory, definition);

  EXPECT_TRUE(discovery.errors.empty()) << to_string(discovery.errors);
  EXPECT_TRUE(discovery.undefinedStepSuggestions.empty());
  ASSERT_EQ(1u, discovery.pickles.size());
  EXPECT_EQ("Calculator", discovery.pickles.front().featureName);
  ASSERT_EQ(1u, discovery.pickles.front().pickle.steps.size());
  EXPECT_EQ("I add 2", discovery.pickles.front().pickle.steps.front().text);
  ASSERT_EQ(1u, discovery.pickles.front().stepLocations.size());
  EXPECT_EQ(4u, discovery.pickles.front().stepLocations.front().line);

  std::filesystem::remove_all(directory);
}

TEST(SourceLocationTest, TracksTheCurrentStep) {
  EXPECT_EQ(nullptr, current_source_location());

  set_current_source_location(
      SourceLocation("calculator.feature", 7, 5, "I add 2"));
  ASSERT_NE(nullptr, current_source_location());
  EXPECT_EQ("calculator.feature:7:5", to_string(*current_source_location()));
  EXPECT_EQ("I add 2", current_source_location()->stepText);

  clear_current_source_location();
  EXPECT_EQ(nullptr, current_source_location());
}

TEST(Discovery, MergesParameterizedUndefinedStepsAcrossFiles) {
  const auto directory = TempDir("undefined-patterns");
  std::filesystem::create_directories(directory);
  {
    std::ofstream feature(std::filesystem::path(directory) / "first.feature");
    feature << "Feature: First\n\n"
               "  Scenario: Adding\n"
               "    When I add items to \"First\"\n"
               "      | price |\n"
               "      | 2     |\n";
  }
  {
    std::ofstream feature(std::filesystem::path(directory) / "second.feature");
    feature << "Feature: Second\n\n"
               "  Scenario: Adding\n"
               "    When I add items to \"Second\"\n"
               "      | price | discount |\n"
               "      | 2.5   | 10       |\n\n"
               "  Scenario: Without a discount\n"
               "    When I add items to \"Third\"\n"
               "      | price |\n"
               "      | 3     |\n";
  }

  const auto discovery = discover_feature_files(directory, StepDefinitions{});
  EXPECT_TRUE(discovery.errors.empty()) << to_string(discovery.errors);
  EXPECT_EQ(R"YAML(types:
  - name: IAddItemsToRow
    kind: struct
    fields:
      - name: price
        type: float
      - name: discount
        type: int
        optional: true

steps:
  - step: ^I add items to "([^"]*)"$
    methodName: i_add_items_to
    arguments:
      - name: arg1
        type: string
    dataTable:
      type: IAddItemsToRow
)YAML",
            discovery.undefinedStepSuggestions);
  std::filesystem::remove_all(directory);
}

} // namespace
