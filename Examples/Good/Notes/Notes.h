#pragma once
#include "INotes.h"
namespace nNotes {
class Notes : public INotes {
public:
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
  void validate_scenario(const Zucchini &zucchini,
                         const cucumber::messages::pickle &pickle,
                         nZucchini::Diagnostics &errors) override {
    (void)pickle;
    bool seenStart = false;
    for (const auto &step : zucchini.steps) {
      auto eMethod = nNotes::step_method(step);
      if (eMethod == StepMethod::startWith) {
        seenStart = true;
      } else if (eMethod == StepMethod::note && !seenStart) {
        nZucchini::add_diagnostic(
            errors, zucchini.uri,
            "doc-string steps must start after a setup step", step.line,
            step.column, nZucchini::DiagnosticSeverity::Error);
      }
    }
  }

private:
  long current = 0;
  std::string lastNote;
  long wrappedSteps = 0;
};
} // namespace nNotes
