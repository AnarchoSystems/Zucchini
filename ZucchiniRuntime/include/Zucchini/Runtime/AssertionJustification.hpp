#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>

namespace nZucchini {
// Fixture-wide totals of asserted facts and of those no business rule claims
// as coverage. The totals are advisory; discovery rejects nothing for them.
struct AssertionJustification {
  std::size_t scenarios = 0;
  std::size_t scenariosWithoutAssertions = 0;
  std::size_t assertedFacts = 0;
  std::size_t unjustifiedFacts = 0;

  void add_scenario(std::size_t asserted, std::size_t unjustified) {
    ++scenarios;
    scenariosWithoutAssertions += asserted == 0 ? 1 : 0;
    assertedFacts += asserted;
    unjustifiedFacts += unjustified;
  }
};

void to_json(nlohmann::json &json, const AssertionJustification &summary);
} // namespace nZucchini
