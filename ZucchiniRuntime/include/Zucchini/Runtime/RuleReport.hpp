#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace nZucchini {
// A report directory is owned by one fixture and build configuration.
std::string rule_result_key(const std::string &fixture,
                            const std::string &testName);
std::string rule_report_build_id(const std::string &executable);
void store_rule_report(const std::string &directory, nlohmann::json report);
std::string rule_report_discovery_id(const std::string &directory);
// Results are isolated per scenario, so independent test processes cannot
// overwrite each other. status="started" records an attempt without claiming
// that it completed.
void store_rule_test_result(const std::string &directory,
                            const nlohmann::json &result);
} // namespace nZucchini
