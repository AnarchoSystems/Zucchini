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

void to_json(nlohmann::json &json, const AssertedFact &fact) {
  json = {{"step_index", fact.step},
          {"line", fact.line},
          {"column", fact.column}};
  if (fact.row) {
    json["cell"] = {{"row", *fact.row}, {"key", fact.key}};
  }
}
} // namespace nZucchini
