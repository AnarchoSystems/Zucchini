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
        long current = 0;
        std::string lastNote;
    };
}
