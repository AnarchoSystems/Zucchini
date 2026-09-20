#pragma once

#include "IInvalidRegexArgument.h"

namespace nInvalidRegexArgument
{
    class InvalidRegexArgument : public IInvalidRegexArgument
    {
    public:
        void enter(long) override
        {
        }
    };
}