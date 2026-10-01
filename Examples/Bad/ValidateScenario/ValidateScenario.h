#pragma once

#include "IValidateScenario.h"

namespace nValidateScenario {
class ValidateScenario : public IValidateScenario {
public:
  void note(const std::string &) override {}

  void validate_scenario(const ScenarioContext &context,
                         nZucchini::Diagnostics &errors) override {
    const auto &step = context.zucchini.steps.front();
    nZucchini::add_diagnostic(errors, context.zucchini.uri,
                              "doc-string steps must start after a setup step",
                              step.line, step.column);
  }
};
} // namespace nValidateScenario