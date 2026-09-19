#pragma once
#include "INotes.h"
namespace nNotes
{
    class Notes : public INotes
    {
    public:
        void startWith(long value) override { current = value; }
        void add(long value) override { current += value; }
        void note(const std::string& value) override { lastNote = value; }
        void resultIs(long value) override { EXPECT_EQ(value, current); }
        void noteIs(const std::string& expected) override { EXPECT_EQ(expected, lastNote); }
    private:
        long current = 0;
        std::string lastNote;
    };
}
