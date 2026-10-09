#pragma once
#include "LegacyStringClass.h"
#include "ILegacyStringDemo.h"
namespace nLegacyStringDemo {
class LegacyStringDemo : public ILegacyStringDemo {
public:
  void addPerson(const nLegacyString::LegacyString &name, long age) override {
    people.push_back(Person{name, age});
  }
  void peopleExist(const std::vector<Person> &rows) override {
    for (const auto &row : rows)
      people.push_back(row);
  }
  void lastNameIs(const nLegacyString::LegacyString &expected) override {
    EXPECT_EQ(expected, people.back().name);
  }
  void personCountIs(long expected) override {
    EXPECT_EQ(expected, static_cast<long>(people.size()));
  }

  void absentNicknames(const std::vector<Person>& rows) override {
    ASSERT_FALSE(rows.empty());
    for (const auto& row : rows) {
      EXPECT_FALSE(row.nickname.has_value());
    }
  }

  void aliases(const std::vector<Aliases>& rows) override {
    ASSERT_EQ(rows.size(), 1u);
    checkAliases(rows.front());
  }
  void jsonAliases(const Aliases& value) override { checkAliases(value); }

private:
  static void checkAliases(const Aliases& value) {
    ASSERT_EQ(value.names.size(), 2u);
    EXPECT_EQ(value.names[0], nLegacyString::LegacyString("Alice"));
    EXPECT_EQ(value.names[1], nLegacyString::LegacyString("Bob"));
    EXPECT_FALSE(value.optionalNames.has_value());
    const auto decoded = nlohmann::json(value).get<Aliases>();
    EXPECT_EQ(decoded.names, value.names);
    EXPECT_FALSE(decoded.optionalNames.has_value());
  }

  std::vector<Person> people;
};
} // namespace nLegacyStringDemo
