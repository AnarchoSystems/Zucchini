#pragma once
#include "IScalarLists.h"

namespace nScalarLists {
class ScalarLists : public IScalarLists {
public:
  void lists(const std::vector<Values>& rows) override { checkRows(rows); }
  void columns(const std::vector<Values>& rows) override { checkRows(rows); }
  void positions(const std::vector<Values>& rows) override { checkRows(rows); }
  void jsonLists(const Values& value) override { check(value); }
  void yamlLists(const Values& value) override { check(value); }

  void absentScalars(const std::vector<OptionalValues>& rows) override {
    ASSERT_EQ(rows.size(), 1u);
    const auto& row = rows.front();
    EXPECT_FALSE(row.count.has_value());
    EXPECT_FALSE(row.amount.has_value());
    EXPECT_FALSE(row.enabled.has_value());
    EXPECT_FALSE(row.label.has_value());
  }
  void presentScalars(const std::vector<OptionalValues>& rows) override {
    ASSERT_EQ(rows.size(), 1u);
    const auto& row = rows.front();
    EXPECT_EQ(row.count, 0);
    EXPECT_EQ(row.amount, 0.0);
    EXPECT_EQ(row.enabled, false);
    EXPECT_EQ(row.label, "text");
  }

  void presentLists(const std::vector<Values>& rows) override {
    ASSERT_EQ(rows.size(), 1u);
    check(rows.front(), true);
  }

private:
  static void checkRows(const std::vector<Values>& rows) {
    ASSERT_EQ(rows.size(), 1u);
    check(rows.front());
  }
  static void check(const Values& value, bool optionalPresent = false) {
    const std::vector<long> integers{2, -3};
    const std::vector<double> decimals{2.5, -3};
    const std::vector<bool> flags{true, false};
    EXPECT_EQ(value.ints, integers);
    EXPECT_EQ(value.integers, integers);
    EXPECT_EQ(value.longs, integers);
    EXPECT_EQ(value.floats, decimals);
    EXPECT_EQ(value.doubles, decimals);
    EXPECT_EQ(value.bools, flags);
    EXPECT_EQ(value.booleans, flags);
    EXPECT_EQ(value.strings, (std::vector<std::string>{"alpha", "beta"}));
    EXPECT_EQ(value.colours, (std::vector<Colour>{Colour::red, Colour::blue}));
    EXPECT_EQ(value.defaultedInts, (std::vector<long>{7, 8}));
    if (optionalPresent) {
      ASSERT_TRUE(value.optionalInts.has_value());
      ASSERT_TRUE(value.optionalStrings.has_value());
      EXPECT_EQ(*value.optionalInts, (std::vector<long>{0, -1}));
      EXPECT_EQ(*value.optionalStrings, (std::vector<std::string>{"one", "two"}));
    } else {
      EXPECT_FALSE(value.optionalInts.has_value());
      EXPECT_FALSE(value.optionalStrings.has_value());
    }
    EXPECT_EQ(nlohmann::json(value).get<Values>().ints, value.ints);
  }
};
}
