#pragma once
#include "Zucchini/Runtime/Reporter.hpp"
#include "Zucchini/Runtime/RequirementDeclarations.hpp"
#include <map>
#include <type_traits>
namespace nZucchini {
template <typename Requirement> class BusinessReporter {
  static_assert(std::is_enum_v<Requirement>, "Requirement must be an enum");

public:
  BusinessReporter(Reporter &reporter,
                   RequirementDeclarations<Requirement> declarations)
      : reporter(reporter) {
    for (auto &[value, metadata] : declarations) {
      if (!requirements.emplace(value, std::move(metadata)).second) {
        throw std::invalid_argument(
            "Each requirement enum value may be declared only once");
      }
    }
  }
  void violation(Requirement requirement, DiagnosticSeverity severity,
                 std::string message, const ZucchiniStep &step) {
    reporter.violation(requirements.at(requirement).name, severity,
                       std::move(message), step);
  }
  void violation(Requirement requirement, DiagnosticSeverity severity,
                 std::string message, const ZucchiniStep &step,
                 const TableCell &cell) {
    reporter.violation(requirements.at(requirement).name, severity,
                       std::move(message), step, cell);
  }
  // Omitting the step attaches an end-of-scenario observation to the scenario
  // declaration.
  void violation(Requirement requirement, DiagnosticSeverity severity,
                 std::string message) {
    reporter.violation(requirements.at(requirement).name, severity,
                       std::move(message));
  }
  void coverage(Requirement requirement, std::string message) {
    reporter.coverage(requirements.at(requirement).name, std::move(message));
  }
  void coverage(Requirement requirement, std::string message,
                const ZucchiniStep &step) {
    reporter.coverage(requirements.at(requirement).name, std::move(message),
                      step);
  }
  // Claims one data table cell as checked evidence for the requirement.
  void coverage(Requirement requirement, std::string message,
                const ZucchiniStep &step, const TableCell &cell) {
    reporter.coverage(requirements.at(requirement).name, std::move(message),
                      step, cell);
  }

private:
  Reporter &reporter;
  std::map<Requirement, RequirementMetadata> requirements;
};
} // namespace nZucchini
