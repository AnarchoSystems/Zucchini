#pragma once

#include <optional>
#include <string>

namespace nZucchini {
// Paths passed by CMake to the generated executable during discovery/runtime.
struct DiscoveryArgs {
  // Directory containing rules.json, the optional HTML report, and
  // per-scenario execution results.
  std::string ruleCoverageReport;
  // Whether discovery also writes the HTML report beside rules.json.
  bool ruleDashboard = false;
  // A stylesheet the HTML report loads after its own styles.
  std::string ruleDashboardStylesheet;
  std::string featureDir;
  std::string manifestDir;
  std::string stepDefinitionsPath;
};

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
      args.ruleDashboard = true;
    } else if (const auto data = read_value("--rule-report-data")) {
      args.ruleCoverageReport = *data;
    } else if (const auto stylesheet =
                   read_value("--rule-coverage-report-css")) {
      args.ruleDashboardStylesheet = *stylesheet;
    } else if (const auto featureDir = read_value("feature_dir")) {
      args.featureDir = *featureDir;
    } else if (const auto manifestDir = read_value("manifest_dir")) {
      args.manifestDir = *manifestDir;
    } else if (const auto path = read_value("step_definitions")) {
      args.stepDefinitionsPath = *path;
    }
  }
  return args;
}
} // namespace nZucchini
