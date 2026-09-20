#include "Zucchini/FeatureParser.hpp"

#include "Zucchini/ManifestParser.hpp"
#include "Zucchini/Naming.hpp"
#include "Zucchini/Snippets.hpp"

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

    std::vector<Zucchini> zucchinis_of(const FeatureParseResult& result)
    {
        std::vector<Zucchini> zucchinis;
        for (const auto& scenario : result.scenarios)
        {
            zucchinis.push_back(scenario.zucchini);
        }
        return zucchinis;
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

        FeatureParseResult result;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), result, errors))
            << to_string(errors);

        const auto zucchinis = zucchinis_of(result);
        ASSERT_EQ(2u, zucchinis.size());
        EXPECT_TRUE(result.undefinedSteps.empty());

        EXPECT_EQ("Calculator", zucchinis[0].featureName);
        EXPECT_EQ("Addition", zucchinis[0].ruleName);
        EXPECT_EQ("Adding two numbers", zucchinis[0].name);
        EXPECT_EQ("calculator.feature", zucchinis[0].uri);
        EXPECT_EQ("Calculator__Addition__Adding_two_numbers", test_name(zucchinis[0]));

        ASSERT_EQ(3u, zucchinis[0].steps.size());
        EXPECT_EQ("startWith", zucchinis[0].steps[0].methodName);
        EXPECT_EQ(1, zucchinis[0].steps[0].captures.at(0).value.get<int>());
        EXPECT_EQ(7u, zucchinis[0].steps[0].line);
        EXPECT_EQ(7u, zucchinis[0].steps[0].column);

        EXPECT_EQ("Subtraction", zucchinis[1].ruleName);
        EXPECT_EQ(14u, zucchinis[1].steps[0].line);
        EXPECT_EQ(-2, zucchinis[1].steps[1].captures.at(0).value.get<int>());

        // The pickle travels along so fixtures can validate scenarios during discovery.
        EXPECT_EQ("Adding two numbers", result.scenarios[0].pickle.name);
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

        FeatureParseResult result;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), result, errors))
            << to_string(errors);

        const auto zucchinis = zucchinis_of(result);
        ASSERT_EQ(1u, zucchinis.size());
        EXPECT_TRUE(zucchinis[0].ruleName.empty());
        ASSERT_EQ(3u, zucchinis[0].steps.size());
        EXPECT_EQ("startWith", zucchinis[0].steps[0].methodName);
        EXPECT_EQ(5u, zucchinis[0].steps[0].line);
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

        FeatureParseResult result;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "calculator.feature", Manifest(), result, errors))
            << to_string(errors);

        const auto zucchinis = zucchinis_of(result);
        ASSERT_EQ(2u, zucchinis.size());
        EXPECT_EQ("Adding", zucchinis[0].name);
        EXPECT_EQ("Adding #2", zucchinis[1].name);
        EXPECT_EQ(1, zucchinis[0].steps[1].captures.at(0).value.get<int>());
        EXPECT_EQ(2, zucchinis[1].steps[1].captures.at(0).value.get<int>());
    }

    TEST(FeatureParser, KeepsSanitizedTestNamesUnique)
    {
        const std::string feature = R"GHERKIN(
Feature: Collisions

  Scenario: Same
    Given I start with 1

  Scenario: Same
    Given I start with 2

  Scenario: Same #2
    Given I start with 3
)GHERKIN";

        FeatureParseResult result;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, "collisions.feature", Manifest(), result, errors))
            << to_string(errors);

        const auto zucchinis = zucchinis_of(result);
        ASSERT_EQ(3u, zucchinis.size());
        EXPECT_EQ("Collisions__Same", test_name(zucchinis[0]));
        EXPECT_EQ("Collisions__Same_2", test_name(zucchinis[1]));
        EXPECT_EQ("Collisions__Same_2_2", test_name(zucchinis[2]));
    }

    TEST(FeatureParser, PreservesWindowsStyleSourcePaths)
    {
        const std::string feature = R"GHERKIN(
Feature: Paths

  Scenario: Windows
    Given I start with 1
)GHERKIN";

        FeatureParseResult result;
        Diagnostics errors;
        ASSERT_TRUE(parse_feature(feature, R"(C:\features\paths.feature)", Manifest(), result, errors))
            << to_string(errors);

        ASSERT_EQ(1u, result.scenarios.size());
        EXPECT_EQ(R"(C:\features\paths.feature)", result.scenarios.front().zucchini.uri);
    }

    TEST(FeatureParser, CollectsUndefinedSteps)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Scenario: Unknown
    Given I start with 1
    When I frobnicate the widget 3 times
    Then the answer should be "42"

  Scenario: Unknown again
    When I frobnicate the widget 4 times
)GHERKIN";

        FeatureParseResult result;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "calculator.feature", Manifest(), result, errors));

        ASSERT_EQ(3u, result.undefinedSteps.size());
        EXPECT_EQ("I frobnicate the widget 3 times", result.undefinedSteps[0].text);
        EXPECT_EQ("the answer should be \"42\"", result.undefinedSteps[1].text);
        EXPECT_EQ("I frobnicate the widget 4 times", result.undefinedSteps[2].text);
    }

    TEST(FeatureParser, CollectsUndefinedStepTableColumns)
    {
        const std::string feature = R"GHERKIN(
Feature: Checkout

  Scenario: Adding items
    When I add the following items:
      | name   | price |
      | Widget | 2.5   |

  Scenario: Adding items with a discount
    When I add the following items:
      | name | price | discount |
      | Gear | 5.0   | 10       |
)GHERKIN";

        FeatureParseResult result;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "checkout.feature", Manifest(), result, errors));

        ASSERT_EQ(1u, result.undefinedSteps.size());
        ASSERT_TRUE(result.undefinedSteps.front().table.has_value());
        const auto& columns = *result.undefinedSteps.front().table;
        ASSERT_EQ(3u, columns.size());
        EXPECT_EQ("name", columns[0].header);
        EXPECT_FALSE(columns[0].optional);
        EXPECT_FALSE(columns[0].couldBeInt);
        EXPECT_FALSE(columns[0].couldBeDouble);
        EXPECT_EQ("price", columns[1].header);
        EXPECT_FALSE(columns[1].optional);
        EXPECT_FALSE(columns[1].couldBeInt);
        EXPECT_TRUE(columns[1].couldBeDouble);
        EXPECT_EQ("discount", columns[2].header);
        EXPECT_TRUE(columns[2].optional);
        EXPECT_TRUE(columns[2].couldBeInt);
        EXPECT_TRUE(columns[2].couldBeDouble);
        EXPECT_FALSE(columns[2].couldBeBool);
    }

    TEST(FeatureParser, ReportsGherkinSyntaxErrorsWithPosition)
    {
        const std::string feature = R"GHERKIN(
Feature: Calculator

  Scenario: Broken
    Given I start with 1
    """
)GHERKIN";

        FeatureParseResult result;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "calculator.feature", Manifest(), result, errors));
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors.front().line.has_value()) << to_string(errors);
    }

      TEST(FeatureParser, ReportsTypedValueErrorsAtStepLocations)
      {
        const std::string feature = R"GHERKIN(
    Feature: Captures

      Scenario: Invalid integer
      Given I start with nope
    )GHERKIN";
        auto manifest = Manifest();
        manifest.steps.front().step = "^I start with (.*)$";

        FeatureParseResult result;
        Diagnostics errors;
        EXPECT_FALSE(parse_feature(feature, "captures.feature", manifest, result, errors));

        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("captures.feature", errors.front().path);
        EXPECT_EQ(5u, errors.front().line);
        EXPECT_EQ(7u, errors.front().column);
        EXPECT_EQ("arguments[0]: cannot parse 'nope' as int", errors.front().message);
      }

    TEST(Snippets, SuggestsDefinitionsForUndefinedSteps)
    {
        const auto snippet = step_snippets({UndefinedStep{"I frobnicate the widget 3 times", std::nullopt},
                                            UndefinedStep{"the answer should be \"42\"", std::nullopt}});

        EXPECT_EQ(R"YAML(steps:
  - step: ^I frobnicate the widget (-?\d+) times$
    methodName: i_frobnicate_the_widget_times
    arguments:
      - name: arg1
        type: int
  - step: ^the answer should be "([^"]*)"$
    methodName: the_answer_should_be
    arguments:
      - name: arg1
        type: string
)YAML",
                  snippet);
    }

    TEST(Snippets, SuggestsFloatArgumentForDecimalNumbers)
    {
        const auto snippet = step_snippet(UndefinedStep{"the cart total is 12.5", std::nullopt});

        EXPECT_EQ(R"YAML(  - step: ^the cart total is (-?\d+\.\d+)$
    methodName: the_cart_total_is
    arguments:
      - name: arg1
        type: float
)YAML",
                  snippet);
    }

    TEST(Snippets, EscapesRegexSpecialCharacters)
    {
        EXPECT_EQ(R"RX(  - step: ^what \(really\)\?$
    methodName: what_really
)RX",
                  step_snippet(UndefinedStep{"what (really)?", std::nullopt}));
    }

    TEST(Snippets, SuggestsTypedTableForUndefinedStep)
    {
        const auto snippet = step_snippets(
            {UndefinedStep{"I add the following items:",
                          std::vector<UndefinedTableColumn>{
                              {"name", false, false, false, false},
                              {"unit price", false, false, true, false},
                              {"discount", true, true, true, false}}}});

        EXPECT_EQ(R"YAML(types:
  - name: IAddTheFollowingItemsRow
    kind: struct
    fields:
      - name: name
      - name: unitPrice
        type: float
        header: "unit price"
      - name: discount
        type: int
        optional: true

steps:
  - step: ^I add the following items:$
    methodName: i_add_the_following_items
    dataTable:
      type: IAddTheFollowingItemsRow
)YAML",
                  snippet);
    }
}
