#pragma once

#include "IMalformedTypedDocString.h"

namespace nMalformedTypedDocString
{
    class MalformedTypedDocString : public IMalformedTypedDocString
    {
    public:
        void noteJson(const Note&) override
        {
        }
    };
}