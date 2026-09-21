#pragma once
#include <map>
#include <optional>
#include <string>
#include "IncludedTypes.h"
#include "ITables.h"

namespace nTables {
class Tables : public ITables {
public:
  void addEntries(const std::vector<Entry> &rows) override { apply(rows); }
  void addEntriesByColumn(const std::vector<Entry> &rows) override {
    apply(rows);
  }
  void addPositionalEntries(const std::vector<Entry> &rows) override {
    apply(rows);
  }
  void addLooseEntries(const std::vector<LooseEntry> &rows) override {
    for (const auto &row : rows) {
      total += row.value;
      for (const auto &[key, value] : row.additionalProperties)
        appendNote(key + "=" + value);
    }
  }
  void addTaggedEntries(const std::vector<TaggedEntry> &rows) override {
    for (const auto &row : rows) {
      total += row.value;
      for (const auto &tag : row.tags) {
        appendNote(to_string(tag));
      }
    }
  }
  void totalIs(long value) override { EXPECT_EQ(value, total); }
  void noteIs(const std::string &expected) override {
    EXPECT_EQ(expected, note);
  }
  void tagSummaryIs(const std::string &expected) override {
    EXPECT_EQ(expected, note);
  }

private:
  void apply(const std::vector<Entry> &rows) {
    for (const auto &row : rows) {
      total += row.value * row.scale;
      if (row.label)
        appendNote(*row.label);
    }
  }
  void appendNote(const std::string &value) {
    if (!note.empty())
      note += ',';
    note += value;
  }
  long total = 0;
  std::string note;
};
} // namespace nTables
