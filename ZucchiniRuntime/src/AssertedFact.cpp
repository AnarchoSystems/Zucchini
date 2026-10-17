#include "Zucchini/Runtime/AssertedFact.hpp"

#include <algorithm>

namespace nZucchini {
namespace {
void locate(AssertedFact &fact, const nlohmann::json &location) {
  if (location.is_object()) {
    fact.line = location.at("line").get<std::uint32_t>();
    fact.column = location.at("column").get<std::uint32_t>();
  }
}

void add_cells(const ZucchiniStep &step, std::size_t stepIndex,
               const DataTableArgument &table,
               std::vector<AssertedFact> &facts) {
  for (std::size_t row = 0; row < table.rows.size(); ++row) {
    const auto &record = table.rows[row];
    const auto *locations =
        row < table.cellLocations.size() ? &table.cellLocations[row] : nullptr;
    // An empty cell still asserts something, such as absence. Ignored columns
    // assert nothing.
    const auto add = [&](std::string key, const nlohmann::json *location) {
      if (std::find(table.ignoredKeys.begin(), table.ignoredKeys.end(), key) !=
          table.ignoredKeys.end()) {
        return;
      }
      AssertedFact fact{stepIndex, row, std::move(key), step.line,
                        step.column};
      if (location != nullptr) {
        locate(fact, *location);
      }
      facts.push_back(std::move(fact));
    };
    if (record.is_object()) {
      for (const auto &cell : record.items()) {
        const nlohmann::json *location = nullptr;
        if (locations != nullptr && locations->is_object()) {
          const auto found = locations->find(cell.key());
          location = found == locations->end() ? nullptr : &*found;
        }
        add(cell.key(), location);
      }
    } else {
      for (std::size_t position = 0; position < record.size(); ++position) {
        add(std::to_string(position),
            locations != nullptr && locations->is_array() &&
                    position < locations->size()
                ? &(*locations)[position]
                : nullptr);
      }
    }
  }
}

std::string describe(const AssertedFact &fact, bool keyed) {
  return (keyed ? "'" + fact.key + "'" : "column " + fact.key) + " in row " +
         std::to_string(*fact.row + 1);
}
} // namespace

std::vector<AssertedFact> asserted_facts(const Zucchini &scenario) {
  std::vector<AssertedFact> facts;
  for (std::size_t index = 0; index < scenario.steps.size(); ++index) {
    const auto &step = scenario.steps[index];
    if (step.keywordType != StepKeywordType::Outcome) {
      continue;
    }
    const auto *table = std::get_if<DataTableArgument>(&step.argument);
    if (table != nullptr && !table->rows.empty()) {
      add_cells(step, index, *table, facts);
    } else {
      facts.push_back({index, std::nullopt, {}, step.line, step.column});
    }
  }
  return facts;
}

std::vector<AssertedFact>
unjustified_facts(const std::vector<AssertedFact> &facts,
                  const JustifiedFacts &justified) {
  std::vector<AssertedFact> result;
  for (const auto &fact : facts) {
    const bool covered = fact.row
                             ? justified.justifies(fact.step, *fact.row,
                                                   fact.key)
                             : justified.justifies(fact.step);
    if (!covered) {
      result.push_back(fact);
    }
  }
  return result;
}

void report_unjustified_facts(const Zucchini &scenario,
                              const std::vector<AssertedFact> &unjustified,
                              DiagnosticSeverity severity,
                              Diagnostics &diagnostics) {
  constexpr std::size_t listed = 5;
  for (std::size_t begin = 0; begin < unjustified.size();) {
    const auto stepIndex = unjustified[begin].step;
    auto end = begin;
    while (end < unjustified.size() && unjustified[end].step == stepIndex) {
      ++end;
    }
    const auto &step = scenario.steps.at(stepIndex);
    std::string message;
    if (!unjustified[begin].row) {
      message = "no business rule justifies this assertion";
    } else {
      const auto *table = std::get_if<DataTableArgument>(&step.argument);
      const bool keyed = table != nullptr && !table->rows.empty() &&
                         table->rows.front().is_object();
      message = "no business rule justifies " +
                std::to_string(end - begin) + " asserted table cell" +
                (end - begin == 1 ? "" : "s") + ": ";
      for (auto index = begin; index < end && index < begin + listed;
           ++index) {
        message += (index == begin ? "" : ", ") +
                   describe(unjustified[index], keyed);
      }
      if (end - begin > listed) {
        message += " and " + std::to_string(end - begin - listed) + " more";
      }
    }
    add_diagnostic(diagnostics, scenario.uri, std::move(message), step.line,
                   step.column, severity);
    begin = end;
  }
}

void enforce_justification(const Zucchini &scenario,
                           const std::vector<AssertedFact> &facts,
                           const std::vector<AssertedFact> &unjustified,
                           const AssertionJustification &settings,
                           Diagnostics &diagnostics) {
  if (facts.empty()) {
    add_diagnostic(diagnostics, scenario.uri,
                   "scenario asserts nothing; add an outcome step such as Then",
                   scenario.line, 0);
    return;
  }
  report_unjustified_facts(scenario, unjustified, settings.severity,
                           diagnostics);
  if (unjustified.size() > settings.maxUnjustifiedFactsPerScenario) {
    add_diagnostic(diagnostics, scenario.uri,
                   "scenario asserts " + std::to_string(unjustified.size()) +
                       " facts that no business rule justifies; at most " +
                       std::to_string(settings.maxUnjustifiedFactsPerScenario) +
                       " are allowed",
                   scenario.line, 0);
  }
}

void to_json(nlohmann::json &json, const AssertedFact &fact) {
  json = {{"step_index", fact.step},
          {"line", fact.line},
          {"column", fact.column}};
  if (fact.row) {
    json["cell"] = {{"row", *fact.row}, {"key", fact.key}};
  }
}
} // namespace nZucchini
