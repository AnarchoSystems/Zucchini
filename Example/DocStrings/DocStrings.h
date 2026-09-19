#pragma once
#include "IDocStrings.h"
namespace nDocStrings
{
    class DocStrings : public IDocStrings
    {
    public:
        void note(const std::string& docString) override { lastNote = docString; priority = 0; }
        void noteJson(const Note& value) override { apply(value); }
        void noteYaml(const Note& value) override { apply(value); }
        void noteIs(const std::string& expected) override { EXPECT_EQ(expected, lastNote); }
        void priorityIs(long expected) override { EXPECT_EQ(expected, priority); }
    private:
        void apply(const Note& value) { lastNote = value.title; priority = value.priority; }
        std::string lastNote;
        long priority = 0;
    };
}
