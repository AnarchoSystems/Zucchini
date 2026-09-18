#include "Zucchini/Zucchini.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
    using namespace nZucchini;

    struct JsonCase
    {
        JsonCase(std::string name, Zucchini value, nlohmann::json expected)
            : name(std::move(name))
            , value(std::move(value))
            , expected(std::move(expected))
        {
        }

        std::string name;
        Zucchini value;
        nlohmann::json expected;

        friend std::ostream& operator<<(std::ostream& stream, const JsonCase& testCase)
        {
            return stream << testCase.name;
        }
    };

    std::string CaseName(const testing::TestParamInfo<JsonCase>& info)
    {
        return info.param.name;
    }

    std::vector<JsonCase> JsonCases()
    {
        return {
            JsonCase("PlainStep",
                     Zucchini("a scenario",
                              "My Feature",
                              {ZucchiniStep("^I do nothing$", "doNothing", "I do nothing", {}, {}, 13, 5)}),
                     nlohmann::json{{"name", "a scenario"},
                                    {"feature", "My Feature"},
                                    {"steps",
                                     {{{"regex", "^I do nothing$"},
                                       {"methodName", "doNothing"},
                                       {"text", "I do nothing"},
                                       {"line", 13},
                                       {"column", 5},
                                       {"captures", nlohmann::json::array()},
                                       {"argument", nullptr}}}}}),

            JsonCase("TypedCaptures",
                     Zucchini("captures",
                              "Captures",
                              {ZucchiniStep("^I have (.*) cukes$",
                                            "haveCukes",
                                            "I have 42 cukes",
                                            {Capture("count", 42), Capture("tasty", true), Capture("label", "Bob")},
                                            {},
                                            14,
                                            3)}),
                     nlohmann::json{{"name", "captures"},
                                    {"feature", "Captures"},
                                    {"steps",
                                     {{{"regex", "^I have (.*) cukes$"},
                                       {"methodName", "haveCukes"},
                                       {"text", "I have 42 cukes"},
                                       {"line", 14},
                                       {"column", 3},
                                       {"captures",
                                        {{{"name", "count"}, {"value", 42}},
                                         {{"name", "tasty"}, {"value", true}},
                                         {{"name", "label"}, {"value", "Bob"}}}},
                                       {"argument", nullptr}}}}}),

            JsonCase("DocStringArgument",
                     Zucchini("docstring",
                              "DocStrings",
                              {ZucchiniStep("^I send a payload$",
                                            "sendPayload",
                                            "I send a payload",
                                            {},
                                            DocStringArgument(R"({"a": 1})", "json"),
                                            15,
                                            3)}),
                     nlohmann::json{{"name", "docstring"},
                                    {"feature", "DocStrings"},
                                    {"steps",
                                     {{{"regex", "^I send a payload$"},
                                       {"methodName", "sendPayload"},
                                       {"text", "I send a payload"},
                                       {"line", 15},
                                       {"column", 3},
                                       {"captures", nlohmann::json::array()},
                                       {"argument",
                                        {{"kind", "docString"},
                                         {"content", R"({"a": 1})"},
                                         {"mediaType", "json"}}}}}}}),

            JsonCase("DataTableArgument",
                     Zucchini("table",
                              "Tables",
                              {ZucchiniStep("^these people exist$",
                                            "peopleExist",
                                            "these people exist",
                                            {},
                                            DataTableArgument({nlohmann::json{{"name", "Alice"}},
                                                               nlohmann::json({"Bob", "40"})}),
                                            16,
                                            3)}),
                     nlohmann::json{{"name", "table"},
                                    {"feature", "Tables"},
                                    {"steps",
                                     {{{"regex", "^these people exist$"},
                                       {"methodName", "peopleExist"},
                                       {"text", "these people exist"},
                                       {"line", 16},
                                       {"column", 3},
                                       {"captures", nlohmann::json::array()},
                                       {"argument",
                                        {{"kind", "dataTable"},
                                         {"rows", {{{"name", "Alice"}}, {"Bob", "40"}}}}}}}}}),
        };
    }

    class ZucchiniJson : public testing::TestWithParam<JsonCase>
    {
    };

    TEST_P(ZucchiniJson, SerializesToExpectedJson)
    {
        EXPECT_EQ(GetParam().expected, nlohmann::json(GetParam().value));
    }

    TEST_P(ZucchiniJson, RoundTrips)
    {
        EXPECT_EQ(GetParam().value, GetParam().expected.get<Zucchini>());
    }

    INSTANTIATE_TEST_SUITE_P(Zucchinis, ZucchiniJson, testing::ValuesIn(JsonCases()), CaseName);
}
