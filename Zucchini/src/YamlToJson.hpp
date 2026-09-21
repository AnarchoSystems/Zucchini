#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace nZucchini {
// Converts a YAML document into JSON; used for YAML DocStrings and schema
// validation.
bool yaml_to_json(const std::string &yaml, nlohmann::json &json,
                  std::string &error);
} // namespace nZucchini
