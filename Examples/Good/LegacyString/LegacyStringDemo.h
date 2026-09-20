#pragma once
#include "ILegacyStringDemo.h"
namespace nLegacyStringDemo
{
    class LegacyStringDemo : public ILegacyStringDemo
    {
    public:
        void addPerson(const nLegacyString::LegacyString& name, long age) override
        {
            people.push_back(Person{name, age});
        }
        void peopleExist(const std::vector<Person>& rows) override
        {
            for (const auto& row : rows) people.push_back(row);
        }
        void lastNameIs(const nLegacyString::LegacyString& expected) override
        {
            EXPECT_EQ(expected, people.back().name);
        }
        void personCountIs(long expected) override { EXPECT_EQ(expected, static_cast<long>(people.size())); }

    private:
        std::vector<Person> people;
    };
}
