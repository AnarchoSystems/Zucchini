#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/AssertionJustification.hpp"
#include "Zucchini/Runtime/JustifiedFacts.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nZucchini {
// A fact a scenario asserts: an Outcome step, or one cell of an Outcome step's
// data table, including empty cells but not ignored columns. DocStrings are
// opaque, so their step is the fact, as is a table step without records.
struct AssertedFact {
  std::size_t step = 0;
  // Present for table cells; key is the cell's header or position.
  std::optional<std::size_t> row;
  std::string key;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
};

std::vector<AssertedFact> asserted_facts(const Zucchini &scenario);

std::vector<AssertedFact>
unjustified_facts(const std::vector<AssertedFact> &facts,
                  const JustifiedFacts &justified);

// One diagnostic per step, at the step, listing its unjustified cells.
void report_unjustified_facts(const Zucchini &scenario,
                              const std::vector<AssertedFact> &unjustified,
                              DiagnosticSeverity severity,
                              Diagnostics &diagnostics);

// Reports unjustified facts, and rejects scenarios without assertions or with
// more unjustified facts than allowed.
void enforce_justification(const Zucchini &scenario,
                           const std::vector<AssertedFact> &facts,
                           const std::vector<AssertedFact> &unjustified,
                           const AssertionJustification &settings,
                           Diagnostics &diagnostics);

void to_json(nlohmann::json &json, const AssertedFact &fact);
} // namespace nZucchini
