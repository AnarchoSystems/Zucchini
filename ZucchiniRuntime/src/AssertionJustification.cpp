#include "Zucchini/Runtime/AssertionJustification.hpp"

namespace nZucchini {
namespace {
const char *severity_name(DiagnosticSeverity severity) {
  switch (severity) {
  case DiagnosticSeverity::Error:
    return "error";
  case DiagnosticSeverity::Warning:
    return "warning";
  case DiagnosticSeverity::Info:
    return "info";
  }
  return "info";
}
} // namespace

void to_json(nlohmann::json &json, const AssertionJustification &summary) {
  json = {{"enforced", summary.enforced},
          {"scenarios", summary.scenarios},
          {"asserted_facts", summary.assertedFacts},
          {"unjustified_facts", summary.unjustifiedFacts}};
  if (summary.enforced) {
    json["severity"] = severity_name(summary.severity);
    json["max_unjustified_facts_per_scenario"] =
        summary.maxUnjustifiedFactsPerScenario;
    json["scenarios_without_assertions"] = summary.scenariosWithoutAssertions;
    json["scenarios_over_limit"] = summary.scenariosOverLimit;
  }
}
} // namespace nZucchini
