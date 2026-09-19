#pragma once
#include "ICalculator.h"
namespace nCalculator
{
    class Calculator : public ICalculator
    {
    public:
        void startWith(long value) override { current = value; }
        void add(long value) override { current += value; }
        void subtract(long value) override { current -= value; }
        void resultIs(long value) override { EXPECT_EQ(value, current); }
        void reset() override { current = 0; }
    private:
        long current = 0;
    };
}
