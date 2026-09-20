#pragma once

#include "ITestAssertion.h"

namespace nTestAssertion
{
    class TestAssertion : public ITestAssertion
    {
    public:
        void startWith(long value) override { current = value; }
        void resultIs(long expected) override { EXPECT_EQ(expected, current); }

    private:
        long current = 0;
    };
}