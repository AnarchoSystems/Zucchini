#pragma once

#include "IRuleValidation.h"

namespace nRuleValidation {
enum class NoteRequirement { SetupBeforeNote };
struct NoteRule : DSLRule<NoteRequirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"Note setup", "Doc-string steps require setup"};
  }
  Requirements requirements() const override {
    return {{NoteRequirement::SetupBeforeNote,
             {"Setup before note",
              "Doc-string steps must start after a setup step."}}};
  }
  void
  validate(const StepContext &context,
           nZucchini::ViolationReporter<NoteRequirement> &reporter) override {
    reporter.violation(NoteRequirement::SetupBeforeNote,
                       nZucchini::DiagnosticSeverity::Error,
                       "doc-string steps must start after a setup step",
                       context.current().step);
  }
};
class RuleValidation : public IRuleValidation {
public:
  void note(const std::string &) override {}
  void register_rules(RuleRegistry &registry) override {
    registry.add<NoteRule>();
  }
};
} // namespace nRuleValidation
