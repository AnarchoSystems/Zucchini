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

std::string name_of(const nZucchini::DataTableRow &row) { return row.at("name"); }
std::string first_cell(const nZucchini::PositionalDataTableRow &cells) {
  return cells.at(0);
}
long length_of(const std::string &text) { return static_cast<long>(text.size()); }

TEST(DataTable, ParsesRowsAndListsWithGeneratedParsers) {
  using namespace nZucchini;
  const ZucchiniStep step(
      "", "", "table", {},
      DataTableArgument({nlohmann::json{{"name", "Ada"}},
                         nlohmann::json{{"name", "Grace"}}}));

  EXPECT_EQ(parse_rows(step, name_of),
            (std::vector<std::string>{"Ada", "Grace"}));
  EXPECT_EQ(parse_positional_rows(step, first_cell),
            (std::vector<std::string>{"Ada", "Grace"}));
  EXPECT_EQ(parse_each({"a", "abc"}, length_of), (std::vector<long>{1, 3}));
}

TEST(DataTable, RejectsStepsWithoutDataTables) {
  const nZucchini::ZucchiniStep step("", "", "no table");
  EXPECT_THROW(nZucchini::DataTable::from_step(step), std::runtime_error);
}
} // namespace