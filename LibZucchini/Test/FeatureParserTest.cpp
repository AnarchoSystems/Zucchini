#include "Zucchini/FeatureParser.hpp"

#include "Zucchini/ManifestParser.hpp"
#include "Zucchini/Naming.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
    using namespace nZucchini;

    const std::string kManifestYaml = R"YAML(
steps:
  - step: ^I start with (-?\d+)$
    methodName: startWith
    arguments:
      - name: value
        type: int
  - step: ^I add (-?\d+)$
    methodName: add
    arguments:
      - name: value
        type: int
  - step: ^the result is (-?\d+)$
    methodName: resultIs
    arguments:
      - name: value
        type: int
)YAML";

    StepDefManifest Manifest()
    {
        StepDefManifest manifest;
        Diagnostics errors;
        EXPECT_TRUE(parse_step_def_manifest(kManifestYaml, manifest, errors)) << to_string(errors);
        return manifest;
    }

    TEST(FeatureParser, ResolvesFeatureRuleAndStepLocations)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Rule: Addition

    Scenario: Adding two numbers
      Given I start with 1
      When I add 2
      Then the result is 3

  Rule: Subtraction

    Scenario: Subtracting two numbers
      Given I start with 5
      When I add -2
      Then the result is 3
)GHERKIN";

        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), zucchinis, errors))
            << to_string(errors);

        ASSERT_EQ(2u, zucchinis.size());

        EXPECT_EQ("Calculator", zucchinis[0].featureName);
        EXPECT_EQ("Addition", zucchinis[0].ruleName);
        EXPECT_EQ("Adding two numbers", zucchinis[0].name);
        EXPECT_EQ("Calculator__Addition__Adding_two_numbers", test_name(zucchinis[0]));

        ASSERT_EQ(3u, zucchinis[0].steps.size());
        EXPECT_EQ("startWith", zucchinis[0].steps[0].methodName);
        EXPECT_EQ("add", zucchinis[0].steps[1].methodName);
        EXPECT_EQ("resultIs", zucchinis[0].steps[2].methodName);
        ASSERT_EQ(1u, zucchinis[0].steps[0].captures.size());
        EXPECT_EQ(1, zucchinis[0].steps[0].captures[0].value.get<int>());

        EXPECT_EQ(7u, zucchinis[0].steps[0].line);
        EXPECT_EQ(8u, zucchinis[0].steps[1].line);
        EXPECT_EQ(9u, zucchinis[0].steps[2].line);
        EXPECT_EQ(7u, zucchinis[0].steps[0].column);

        EXPECT_EQ("Subtraction", zucchinis[1].ruleName);
        EXPECT_EQ("Subtracting two numbers", zucchinis[1].name);
        EXPECT_EQ(14u, zucchinis[1].steps[0].line);
        EXPECT_EQ(-2, zucchinis[1].steps[1].captures[0].value.get<int>());
    }

    TEST(FeatureParser, IncludesBackgroundSteps)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Background:
    Given I start with 10

  Scenario: Adding
    When I add 5
    Then the result is 15
)GHERKIN";

        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), zucchinis, errors))
            << to_string(errors);

        ASSERT_EQ(1u, zucchinis.size());
        EXPECT_TRUE(zucchinis[0].ruleName.empty());
        ASSERT_EQ(3u, zucchinis[0].steps.size());
        EXPECT_EQ("startWith", zucchinis[0].steps[0].methodName);
        EXPECT_EQ(5u, zucchinis[0].steps[0].line);
        EXPECT_EQ(8u, zucchinis[0].steps[1].line);
    }

    TEST(FeatureParser, ExpandsScenarioOutlinesAndKeepsNamesUnique)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Scenario Outline: Adding
    Given I start with 0
    When I add <count>
    Then the result is <count>

    Examples:
      | count |
      | 1     |
      | 2     |
)GHERKIN";

        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), zucchinis, errors))
            << to_string(errors);

        ASSERT_EQ(2u, zucchinis.size());
        EXPECT_EQ("Adding", zucchinis[0].name);
        EXPECT_EQ("Adding #2", zucchinis[1].name);
        EXPECT_NE(test_name(zucchinis[0]), test_name(zucchinis[1]));
        EXPECT_EQ(1, zucchinis[0].steps[1].captures[0].value.get<int>());
        EXPECT_EQ(2, zucchinis[1].steps[1].captures[0].value.get<int>());
    }

    TEST(FeatureParser, ReportsUnmatchedSteps)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Scenario: Unknown
    Given I do something nobody declared
)GHERKIN";

        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "calculator.feature", Manifest(), zucchinis, errors));
        ASSERT_FALSE(errors.empty());
        EXPECT_EQ("calculator.feature[0].steps[0]", to_string(errors.front().path));
        EXPECT_TRUE(zucchinis.empty());
    }

    TEST(FeatureParser, ReportsGherkinSyntaxErrorsWithPosition)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Scenario: Broken
    Given I start with 1
    """
)GHERKIN";

        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "calculator.feature", Manifest(), zucchinis, errors));
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors.front().line.has_value()) << to_string(errors);
    }
}
