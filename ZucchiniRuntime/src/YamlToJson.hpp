#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace nZucchini {
bool yaml_to_json(const std::string &yaml, nlohmann::json &json,
                  std::string &error);
} // namespace nZucchini