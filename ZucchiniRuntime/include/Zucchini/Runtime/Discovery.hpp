#pragma once

#include "Zucchini/Runtime/AssertionJustification.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace nZucchini {
// Paths passed by CMake to the generated executable during discovery/runtime.
struct DiscoveryArgs {
  // Directory containing rules.json, index.html, and per-scenario execution results.
  std::string ruleCoverageReport;
  std::string featureDir;
  std::string manifestDir;
  std::string stepDefinitionsPath;
  // Enforced when asserted facts must be justified by business rules.
  AssertionJustification justification;
};

inline DiagnosticSeverity parse_diagnostic_severity(const std::string &name) {
  if (name == "error") {
    return DiagnosticSeverity::Error;
  }
  if (name == "warning") {
    return DiagnosticSeverity::Warning;
  }
  if (name == "info") {
    return DiagnosticSeverity::Info;
  }
  throw std::invalid_argument("unknown diagnostic severity '" + name +
                              "'; expected error, warning or info");
}

inline std::size_t parse_count(const std::string &text) {
  if (text.empty() ||
      text.find_first_not_of("0123456789") != std::string::npos) {
    throw std::invalid_argument("expected a nonnegative integer, got '" +
                                text + "'");
  }
  return static_cast<std::size_t>(std::stoull(text));
}

inline DiscoveryArgs parse_discovery_args(int argc, char **argv) {
  DiscoveryArgs args;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    const auto read_value = [&](const std::string &key) -> std::optional<std::string> {
      const auto prefix = key + '=';
      if (argument.rfind(prefix, 0) == 0) {
        return argument.substr(prefix.size());
      }
      return std::nullopt;
    };
    if (const auto report = read_value("--rule-coverage-report")) {
      args.ruleCoverageReport = *report;
    } else if (const auto featureDir = read_value("feature_dir")) {
      args.featureDir = *featureDir;
    } else if (const auto manifestDir = read_value("manifest_dir")) {
      args.manifestDir = *manifestDir;
    } else if (const auto path = read_value("step_definitions")) {
      args.stepDefinitionsPath = *path;
    } else if (const auto severity =
                   read_value("--unjustified-assertions")) {
      args.justification.enforced = true;
      args.justification.severity = parse_diagnostic_severity(*severity);
    } else if (const auto limit =
                   read_value("--max-unjustified-facts-per-scenario")) {
      args.justification.enforced = true;
      args.justification.maxUnjustifiedFactsPerScenario = parse_count(*limit);
    }
  }
  return args;
}
} // namespace nZucchini
