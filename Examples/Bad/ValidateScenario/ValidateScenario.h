#pragma once

#include "IValidateScenario.h"

namespace nValidateScenario
{
    class ValidateScenario : public IValidateScenario
    {
    public:
        void note(const std::string&) override
        {
        }

        void validate_scenario(const Zucchini& zucchini,
                               const cucumber::messages::pickle&,
                               nZucchini::Diagnostics& errors) override
        {
            const auto& step = zucchini.steps.front();
            nZucchini::add_diagnostic(errors,
                                      zucchini.uri,
                                      "doc-string steps must start after a setup step",
                                      step.line,
                                      step.column);
        }
    };
}