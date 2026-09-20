#pragma once

#include "IMalformedTable.h"

namespace nMalformedTable
{
    class MalformedTable : public IMalformedTable
    {
    public:
        void addEntries(const std::vector<Entry>&) override
        {
        }
    };
}