#pragma once

#include "IWrongTypedDocStringValue.h"

namespace nWrongTypedDocStringValue
{
    class WrongTypedDocStringValue : public IWrongTypedDocStringValue
    {
    public:
        void noteJson(const Note&) override
        {
        }
    };
}