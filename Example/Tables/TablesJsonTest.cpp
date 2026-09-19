#include "ITables.h"
#include "Tables.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(TablesJson, ImportedStructRoundTrips)
{
    nTables::Entry original;
    original.value = 7;
    original.label = "seven";
    original.scale = 3;

    nlohmann::json encoded = original;
    EXPECT_EQ(encoded["value"], 7);
    EXPECT_EQ(encoded["label"], "seven");
    EXPECT_EQ(encoded["scale"], 3);

    const auto decoded = encoded.get<nTables::Entry>();
    EXPECT_EQ(decoded.value, original.value);
    EXPECT_EQ(decoded.label, original.label);
    EXPECT_EQ(decoded.scale, original.scale);
}
