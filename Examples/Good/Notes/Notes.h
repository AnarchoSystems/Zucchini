#pragma once
#include "INotes.h"
namespace nNotes {
enum class NoteRequirement { AfterSetup, HasNote };
struct NoteRule : BusinessRule<NoteRequirement> {
  bool seenStart = false;
  nZucchini::RuleMetadata metadata() const override {
    return {"note-order", "Notes require a setup step"};
  }
  Requirements requirements() const override {
    return {
        {NoteRequirement::HasNote, {"Has A Note", "Scenario contains a note at all"}},
        {NoteRequirement::AfterSetup, {"Note Comes After Setup", "Note is only valid after a setup step"}}};
  }
  void
  validate(const StepContext &context,
           nZucchini::BusinessReporter<NoteRequirement> &reporter) override {
    const auto step = context.current();
    if (step.method == StepMethod::startWith) {
      seenStart = true;
    } else if (step.method == StepMethod::note) {
      reporter.coverage(NoteRequirement::HasNote, "Note found", step.step);
      if (seenStart) {
        reporter.coverage(NoteRequirement::AfterSetup, "Setup precedes note",
                          step.step);
      } else {
        reporter.violation(NoteRequirement::AfterSetup,
                           nZucchini::DiagnosticSeverity::Error,
                           "Note requires setup", step.step);
      }
    }
  }
};
class Notes : public INotes {
public:
  void register_rules(RuleRegistry &registry) override {
    registry.add<NoteRule>();
  }
  void startWith(long value) override { current = value; }
  void add(long value) override { current += value; }
  void note(const std::string &value) override { lastNote = value; }
  void resultIs(long value) override { EXPECT_EQ(value, current); }
  void noteIs(const std::string &expected) override {
    EXPECT_EQ(expected, lastNote);
  }
  void wrappedStepsAre(long expected) override {
    EXPECT_EQ(expected, wrappedSteps);
  }
  void around_step(const StepContext &context,
                   const std::function<void()> &step) override {
    (void)context;
    ++wrappedSteps;
    step();
  }

private:
  long current = 0;
  std::string lastNote;
  long wrappedSteps = 0;
};
} // namespace nNotes
