#include "Zucchini/Discovery.hpp"

#include "Zucchini/ManifestStore.hpp"
#include "Zucchini/Naming.hpp"
#include "Zucchini/SourceLocation.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace
{
    using namespace nZucchini;

    Zucchini Sample(std::string scenario, std::string rule = {})
    {
        return Zucchini(std::move(scenario),
                        "Calculator",
                        {ZucchiniStep("^I add (\\d+)$", "add", "I add 2", {Capture("value", 2)}, {}, 7, 5)},
                        std::move(rule));
    }

    std::string TempDir(const std::string& name)
    {
        const auto path = std::filesystem::temp_directory_path() / ("zucchini-test-" + name);
        std::filesystem::remove_all(path);
        return path.string();
    }

    TEST(ManifestStore, RoundTripsZucchinisByTestName)
    {
        const auto directory = TempDir("store");
        const std::vector<Zucchini> zucchinis = {Sample("Adding", "Addition"), Sample("Subtracting")};

        Diagnostics errors;
        ASSERT_TRUE(store_zucchinis(directory, zucchinis, errors)) << to_string(errors);
        EXPECT_TRUE(std::filesystem::exists(manifest_path(directory, test_name(zucchinis[0]))));

        Zucchini single;
        ASSERT_TRUE(load_zucchini(directory, test_name(zucchinis[0]), single, errors)) << to_string(errors);
        EXPECT_EQ(zucchinis[0], single);

        std::vector<Zucchini> loaded;
        ASSERT_TRUE(load_zucchinis(directory, loaded, errors)) << to_string(errors);
        ASSERT_EQ(2u, loaded.size());

        std::filesystem::remove_all(directory);
    }

    TEST(ManifestStore, ReportsMissingManifests)
    {
        const auto directory = TempDir("missing");

        Zucchini zucchini;
        Diagnostics errors;
        EXPECT_FALSE(load_zucchini(directory, "Nope", zucchini, errors));
        ASSERT_FALSE(errors.empty());
        EXPECT_NE(std::string::npos, errors.front().message.find("re-run test discovery"));
    }

    TEST(Discovery, ParsesDiscoveryArguments)
    {
        const char* argv[] = {"test", "--gtest_list_tests", "feature_dir=/features", "manifest_dir=/manifests"};

        const auto args = parse_discovery_args(4, const_cast<char**>(argv));

        EXPECT_EQ("/features", args.featureDir);
        EXPECT_EQ("/manifests", args.manifestDir);
    }

    TEST(Discovery, LoadsParametersLazilyFromTheProvider)
    {
        int calls = 0;
        set_zucchini_provider([&calls] {
            ++calls;
            return std::vector<Zucchini>{Sample("Adding", "Addition"), Sample("Subtracting")};
        });

        const auto values = zucchini_values();
        EXPECT_EQ(0, calls) << "the provider must not run before the generator is iterated";

        std::vector<std::string> names;
        for (auto value = values.begin(); value != values.end(); ++value)
        {
            names.push_back(test_name(*value));
        }

        EXPECT_EQ(1, calls);
        ASSERT_EQ(2u, names.size());
        EXPECT_EQ("Calculator__Addition__Adding", names[0]);
        EXPECT_EQ("Calculator__Subtracting", names[1]);

        set_zucchini_provider(nullptr);
    }

    TEST(SourceLocationTest, TracksTheCurrentStep)
    {
        EXPECT_EQ(nullptr, current_source_location());

        set_current_source_location(SourceLocation("calculator.feature", 7, 5, "I add 2"));
        ASSERT_NE(nullptr, current_source_location());
        EXPECT_EQ("calculator.feature:7:5", to_string(*current_source_location()));
        EXPECT_EQ("I add 2", current_source_location()->stepText);

        clear_current_source_location();
        EXPECT_EQ(nullptr, current_source_location());
    }
}
