#include "Zucchini/Naming.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using namespace nZucchini;

struct NameCase {
  NameCase(std::string name, std::string feature, std::string rule,
           std::string scenario, std::string expectedTestName,
           std::string expectedDisplayName)
      : name(std::move(name)), feature(std::move(feature)),
        rule(std::move(rule)), scenario(std::move(scenario)),
        expectedTestName(std::move(expectedTestName)),
        expectedDisplayName(std::move(expectedDisplayName)) {}

  std::string name;
  std::string feature;
  std::string rule;
  std::string scenario;
  std::string expectedTestName;
  std::string expectedDisplayName;

  friend std::ostream &operator<<(std::ostream &stream, const NameCase &value) {
    return stream << value.name;
  }
};

std::string CaseName(const testing::TestParamInfo<NameCase> &info) {
  return info.param.name;
}

std::vector<NameCase> NameCases() {
  return {
      NameCase("WithRule", "Shopping cart", "Empty carts", "Adding one item",
               "Shopping_cart__Empty_carts__Adding_one_item",
               "Shopping cart: Empty carts: Adding one item"),

      NameCase("WithoutRule", "Shopping cart", "", "Adding one item",
               "Shopping_cart__Adding_one_item",
               "Shopping cart: Adding one item"),

      NameCase("TransliteratesUmlauts", "Gr\u00fc\u00dfe", "",
               "\u00c4pfel z\u00e4hlen", "Gruesse__Aepfel_zaehlen",
               "Gr\u00fc\u00dfe: \u00c4pfel z\u00e4hlen"),

      NameCase("DropsPunctuationAndCollapsesSeparators", "Check-out (fast!)",
               "", "1 + 1 = 2", "Check_out_fast__1_1_2",
               "Check-out (fast!): 1 + 1 = 2"),

      NameCase("FallsBackForEmptySegments", "???", "", "!!!",
               "Unnamed__Unnamed", "???: !!!"),
  };
}

class Naming : public testing::TestWithParam<NameCase> {};

TEST_P(Naming, BuildsTestName) {
  EXPECT_EQ(
      GetParam().expectedTestName,
      test_name(GetParam().feature, GetParam().rule, GetParam().scenario));
}

TEST_P(Naming, BuildsDisplayName) {
  EXPECT_EQ(
      GetParam().expectedDisplayName,
      display_name(GetParam().feature, GetParam().rule, GetParam().scenario));
}

INSTANTIATE_TEST_SUITE_P(Names, Naming, testing::ValuesIn(NameCases()),
                         CaseName);

TEST(Naming, NamesZucchinisDirectly) {
  const Zucchini zucchini("Adding one item", "Shopping cart", {},
                          "Empty carts");

  EXPECT_EQ("Shopping_cart__Empty_carts__Adding_one_item", test_name(zucchini));
  EXPECT_EQ("Shopping cart: Empty carts: Adding one item",
            display_name(zucchini));
}
} // namespace
