#pragma once

#include "ITableHeaderTypo.h"

namespace nTableHeaderTypo
{
    class TableHeaderTypo : public ITableHeaderTypo
    {
    public:
        void addEntries(const std::vector<Entry>&) override
        {
        }
    };
}