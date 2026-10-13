#pragma once
#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace nZucchini {
class Reporter {
public:
  Reporter(Diagnostics &diagnostics, nlohmann::json *events,
           const Zucchini &scenario,
           std::optional<std::size_t> scenarioId = std::nullopt)
      : diagnostics(diagnostics), events(events), scenario(scenario),
        scenarioId(scenarioId) {}

  void violation(const std::string &tag, DiagnosticSeverity severity,
                 std::string message, const ZucchiniStep &step) {
    add_diagnostic(diagnostics, scenario.uri, message, step.line, step.column,
                   severity);
    record("violation", tag, message, step, severity);
  }
  void coverage(const std::string &tag, std::string message,
                const ZucchiniStep &step) {
    record("coverage", tag, message, step, std::nullopt);
  }

  void violation(const std::string &tag, DiagnosticSeverity severity,
                 std::string message) {
    ZucchiniStep location;
    location.line = scenario.line;
    violation(tag, severity, std::move(message), location);
  }
  void coverage(const std::string &tag, std::string message) {
    ZucchiniStep location;
    location.line = scenario.line;
    coverage(tag, std::move(message), location);
  }

private:
  void record(const char *kind, const std::string &tag,
              const std::string &message, const ZucchiniStep &step,
              std::optional<DiagnosticSeverity> severity) {
    if (!events) {
      return;
    }
    nlohmann::json event = {{"kind", kind},
                            {"tag", tag},
                            {"message", message},
                            {"scenario", scenario.name},
                            {"feature", scenario.featureName},
                            {"gherkin_rule", scenario.ruleName},
                            {"scenario_line", scenario.line},
                            {"scenario_tags", scenario.tags},
                            {"uri", scenario.uri},
                            {"line", step.line},
                            {"column", step.column},
                            {"step", step.text}};
    if (scenarioId) {
      event["scenario_id"] = *scenarioId;
    }
    if (severity) {
      event["severity"] = *severity == DiagnosticSeverity::Error     ? "error"
                          : *severity == DiagnosticSeverity::Warning ? "warning"
                                                                     : "info";
    }
    events->push_back(std::move(event));
  }
  Diagnostics &diagnostics;
  nlohmann::json *events;
  const Zucchini &scenario;
  std::optional<std::size_t> scenarioId;
};
} // namespace nZucchini
