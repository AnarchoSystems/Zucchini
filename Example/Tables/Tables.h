#pragma once
#include "ITables.h"
#include <map>
#include <optional>
#include <string>
namespace nTables
{
    struct Entry { long value = 0; std::optional<std::string> label; long scale = 1; };
    struct LooseEntry { long value = 0; std::map<std::string, std::string> additionalProperties; };
    class Tables : public ITables
    {
    public:
        void addEntries(const std::vector<Entry>& rows) override { apply(rows); }
        void addEntriesByColumn(const std::vector<Entry>& rows) override { apply(rows); }
        void addPositionalEntries(const std::vector<Entry>& rows) override { apply(rows); }
        void addLooseEntries(const std::vector<LooseEntry>& rows) override
        {
            for (const auto& row : rows) {
                total += row.value;
                for (const auto& [key, value] : row.additionalProperties) appendNote(key + "=" + value);
            }
        }
        void totalIs(long value) override { EXPECT_EQ(value, total); }
        void noteIs(const std::string& expected) override { EXPECT_EQ(expected, note); }
    private:
        void apply(const std::vector<Entry>& rows)
        {
            for (const auto& row : rows) {
                total += row.value * row.scale;
                if (row.label) appendNote(*row.label);
            }
        }
        void appendNote(const std::string& value) { if (!note.empty()) note += ','; note += value; }
        long total = 0;
        std::string note;
    };
}
