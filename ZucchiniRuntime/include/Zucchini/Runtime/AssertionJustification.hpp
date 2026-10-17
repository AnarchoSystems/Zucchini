#pragma once

#include "Zucchini/Diagnostics.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>

namespace nZucchini {
// Settings and fixture-wide totals of the check that every asserted fact is
// justified by business rule coverage. Reports record asserted facts even when
// the check is not enforced.
struct AssertionJustification {
  // When enforced, unjustified facts are diagnosed with the given severity, and
  // scenarios without assertions or above the limit are rejected.
  bool enforced = false;
  DiagnosticSeverity severity = DiagnosticSeverity::Warning;
  std::size_t maxUnjustifiedFactsPerScenario = 2;

  std::size_t scenarios = 0;
  std::size_t scenariosWithoutAssertions = 0;
  std::size_t scenariosOverLimit = 0;
  std::size_t assertedFacts = 0;
  std::size_t unjustifiedFacts = 0;

  void add_scenario(std::size_t asserted, std::size_t unjustified) {
    ++scenarios;
    scenariosWithoutAssertions += asserted == 0 ? 1 : 0;
    scenariosOverLimit += unjustified > maxUnjustifiedFactsPerScenario ? 1 : 0;
    assertedFacts += asserted;
    unjustifiedFacts += unjustified;
  }
};

void to_json(nlohmann::json &json, const AssertionJustification &summary);
} // namespace nZucchini
