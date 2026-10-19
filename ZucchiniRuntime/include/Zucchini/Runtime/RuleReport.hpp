#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace nZucchini {
// Wire-format metadata comes from the selected versioned JSON schema.
const char *rule_report_schema_version();
const char *rule_report_schema_uri();
// A report directory is owned by one fixture and build configuration.
std::string rule_result_key(const std::string &fixture,
                            const std::string &testName);
std::string rule_report_build_id(const std::string &executable);
struct RuleDashboardOptions {
  // Without the HTML report, discovery removes report files it wrote earlier.
  bool enabled = true;
  // Copied beside the report, which loads it after its own styles.
  std::string stylesheet;
};
// Writes rules.json and, when enabled, the HTML report.
void store_rule_report(const std::string &directory, nlohmann::json report,
                       const RuleDashboardOptions &dashboard = {});
std::string rule_report_discovery_id(const std::string &directory);
// Results are isolated per scenario, so independent test processes cannot
// overwrite each other. status="started" records an attempt without claiming
// that it completed.
void store_rule_test_result(const std::string &directory,
                            const nlohmann::json &result);
} // namespace nZucchini
