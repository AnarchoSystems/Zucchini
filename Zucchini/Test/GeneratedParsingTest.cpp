#include "IRegression.h"
#include <gtest/gtest.h>

TEST(GeneratedParsing, ExcludesAllKnownHeadersFromExtras) {
  const auto value = nRegression::parse_Record(
      {{"ignored", "discard"}, {"skipped", "discard"},
       {"value", "primary"}, {"known", "alias"}, {"extra", "keep"}});
  EXPECT_EQ(value.value, "primary");
  EXPECT_EQ(value.additionalProperties,
            (std::map<std::string, std::string>{{"extra", "keep"}}));
}

TEST(GeneratedParsing, PreservesIgnoredColumnPositions) {
  const auto value = nRegression::parse_positional_Positional({"discard", "actual"});
  EXPECT_EQ(value.value, "actual");
}

TEST(GeneratedParsing, RetainsJsonExtrasAndExcludesIgnoredFields) {
  const nlohmann::json input = {{"value", "known"}, {"skipped", "discard"},
                                {"extra", "keep"}};
  const auto value = input.get<nRegression::Record>();
  EXPECT_EQ(value.additionalProperties,
            (std::map<std::string, std::string>{{"extra", "keep"}}));
}

TEST(GeneratedParsing, SerializesExtrasWithoutOverwritingKnownFields) {
  nRegression::Record value{};
  value.value = "known";
  value.additionalProperties = {{"extra", "keep"}, {"value", "wrong"},
                                {"skipped", "discard"}};
  EXPECT_EQ(nlohmann::json(value),
            (nlohmann::json{{"value", "known"}, {"extra", "keep"}}));
}

TEST(GeneratedParsing, UsesDefaultsForEmptyCells) {
  EXPECT_EQ(nRegression::parse_Defaulted({{"count", ""}}).count, 7);
  EXPECT_EQ(nRegression::parse_Defaulted({}).count, 7);
  EXPECT_EQ(nRegression::parse_Defaulted({{"count", "3"}}).count, 3);
}
