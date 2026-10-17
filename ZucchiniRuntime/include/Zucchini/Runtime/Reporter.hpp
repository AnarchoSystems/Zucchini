#pragma once
#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/JustifiedFacts.hpp"
#include "Zucchini/Runtime/TableCell.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace nZucchini {
class Reporter {
public:
  Reporter(Diagnostics &diagnostics, nlohmann::json *events,
           const Zucchini &scenario,
           std::optional<std::size_t> scenarioId = std::nullopt,
           JustifiedFacts *justified = nullptr)
      : diagnostics(diagnostics), events(events), scenario(scenario),
        scenarioId(scenarioId), justified(justified) {}

  void violation(const std::string &tag, DiagnosticSeverity severity,
                 std::string message, const ZucchiniStep &step) {
    add_diagnostic(diagnostics, scenario.uri, message, step.line, step.column,
                   severity);
    record("violation", tag, message, step, severity);
  }
  // A cell the table does not contain is reported at its step.
  void violation(const std::string &tag, DiagnosticSeverity severity,
                 std::string message, const ZucchiniStep &step,
                 const TableCell &cell) {
    const auto target = cell_event(step, cell);
    add_diagnostic(diagnostics, scenario.uri, message,
                   target.value("line", step.line),
                   target.value("column", step.column), severity);
    record("violation", tag, message, step, severity, target);
  }
  // Justifies the step as an asserted fact. Steps with a data table assert
  // their cells instead; justify those with the cell overload.
  void coverage(const std::string &tag, std::string message,
                const ZucchiniStep &step) {
    if (justified) {
      if (const auto index = step_index(step)) {
        justified->justify(*index);
      }
    }
    record("coverage", tag, message, step, std::nullopt);
  }
  // Justifies one asserted table cell. A cell the table does not contain, such
  // as an absent optional column, justifies nothing.
  void coverage(const std::string &tag, std::string message,
                const ZucchiniStep &step, const TableCell &cell) {
    const auto target = cell_event(step, cell);
    if (justified && target.contains("key")) {
      if (const auto index = step_index(step)) {
        justified->justify(*index, cell.row,
                           target.at("key").get<std::string>());
      }
    }
    record("coverage", tag, message, step, std::nullopt, target);
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
    record("coverage", tag, message, location, std::nullopt);
  }

private:
  std::optional<std::size_t> step_index(const ZucchiniStep &step) const {
    const std::less<const ZucchiniStep *> before;
    const auto *first = scenario.steps.data();
    const auto *last = first + scenario.steps.size();
    if (!before(&step, first) && before(&step, last)) {
      return static_cast<std::size_t>(&step - first);
    }
    // Rules may pass copies of the scenario's steps.
    for (std::size_t index = 0; index < scenario.steps.size(); ++index) {
      if (scenario.steps[index] == step) {
        return index;
      }
    }
    return std::nullopt;
  }

  static nlohmann::json cell_event(const ZucchiniStep &step,
                                   const TableCell &cell) {
    nlohmann::json target = {{"row", cell.row}};
    const auto key = cell.key(step);
    if (!key) {
      return target;
    }
    target["key"] = *key;
    const auto &table = std::get<DataTableArgument>(step.argument);
    if (cell.row < table.cellLocations.size()) {
      const auto &locations = table.cellLocations[cell.row];
      const auto location = locations.is_object()
                                ? locations.find(*key)
                                : locations.end();
      if (location != locations.end() && location->is_object()) {
        target["line"] = location->at("line");
        target["column"] = location->at("column");
      } else if (locations.is_array()) {
        const auto position = std::stoul(*key);
        if (position < locations.size() && locations[position].is_object()) {
          target["line"] = locations[position].at("line");
          target["column"] = locations[position].at("column");
        }
      }
    }
    return target;
  }

  void record(const char *kind, const std::string &tag,
              const std::string &message, const ZucchiniStep &step,
              std::optional<DiagnosticSeverity> severity,
              std::optional<nlohmann::json> cell = std::nullopt) {
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
    if (cell) {
      event["cell"] = std::move(*cell);
    }
    events->push_back(std::move(event));
  }
  Diagnostics &diagnostics;
  nlohmann::json *events;
  const Zucchini &scenario;
  std::optional<std::size_t> scenarioId;
  JustifiedFacts *justified;
};
} // namespace nZucchini
