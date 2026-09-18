#pragma once

#include "ICalculator.h"

namespace nCalculator
{
    // The fixture the generated test file expects by convention.
    class Calculator : public ICalculator
    {
    public:
        void startWith(long value) override
        {
            current = value;
        }

        void add(long value) override
        {
            current += value;
        }

        void addAll(const std::vector<Row>& rows) override
        {
            for (const auto& row : rows)
            {
                current += to_long(require_cell(row, "value"));
            }
        }

        void addEntries(const std::vector<Entry>& rows) override
        {
            applyEntries(rows);
        }

        void addEntriesByColumn(const std::vector<Entry>& rows) override
        {
            applyEntries(rows);
        }

        void addPositionalEntries(const std::vector<Entry>& rows) override
        {
            applyEntries(rows);
        }

        void addLooseEntries(const std::vector<LooseEntry>& rows) override
        {
            for (const auto& row : rows)
            {
                current += row.value;
                for (const auto& extra : row.additionalProperties)
                {
                    appendNote(extra.first + "=" + extra.second);
                }
            }
        }

        void addRawNumbers(const std::vector<std::vector<std::string>>& rows) override
        {
            for (const auto& row : rows)
            {
                for (const auto& cell : row)
                {
                    if (!cell.empty())
                    {
                        current += to_long(cell);
                    }
                }
            }
        }

        void note(const std::string& docString) override
        {
            lastNote = docString;
        }

        void resultIs(long value) override
        {
            EXPECT_EQ(value, current);
        }

        void noteIs(const std::string& expected) override
        {
            EXPECT_EQ(expected, lastNote);
        }

    private:
        void applyEntries(const std::vector<Entry>& rows)
        {
            for (const auto& row : rows)
            {
                current += row.value * row.scale;
                if (row.label)
                {
                    appendNote(*row.label);
                }
            }
        }

        void appendNote(const std::string& text)
        {
            if (!lastNote.empty())
            {
                lastNote += ',';
            }
            lastNote += text;
        }

        long current = 0;
        std::string lastNote;
    };
}
