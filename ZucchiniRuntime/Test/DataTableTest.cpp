#include <Zucchini/Runtime/DataTable.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
struct CustomString {
  explicit CustomString(const char *value) : value(value) {}

  std::string value;

  friend bool operator<(const CustomString &lhs, const CustomString &rhs) {
    return lhs.value < rhs.value;
  }
};

TEST(DataTable, NormalizesRowsAndMaterializesCustomStrings) {
  using namespace nZucchini;
  const ZucchiniStep step(
      "", "", "table", {},
      DataTableArgument({nlohmann::json{{"name", "Ada"}, {"age", 37}},
                         nlohmann::json({"Grace", 85})}));

  const auto table = DataTable::from_step(step);
  ASSERT_EQ(table.rows.size(), 2);
  EXPECT_EQ(table.rows[0].at("name"), "Ada");
  EXPECT_EQ(table.rows[0].at("age"), "37");
  EXPECT_EQ(table.rows[1].at("0"), "Grace");
  EXPECT_EQ(table.positionalRows[0],
            (PositionalDataTableRow{"37", "Ada"}));

  const auto dictionaries = table.dictionary_rows<CustomString>();
  ASSERT_EQ(dictionaries.size(), 2);
  EXPECT_EQ(dictionaries[0].at(CustomString("name")).value, "Ada");

  const auto positional = table.positional_rows<CustomString>();
  ASSERT_EQ(positional.size(), 2);
  EXPECT_EQ(positional[1][0].value, "Grace");
}

TEST(DataTable, RejectsStepsWithoutDataTables) {
  const nZucchini::ZucchiniStep step("", "", "no table");
  EXPECT_THROW(nZucchini::DataTable::from_step(step), std::runtime_error);
}
} // namespace