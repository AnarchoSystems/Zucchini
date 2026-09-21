#pragma once

#include <Zucchini/Diagnostics.hpp>

#include <nlohmann/json.hpp>

namespace nZucchini {
// Validates a manifest document against schema.json before it is parsed.
bool validate_against_schema(const nlohmann::json &document,
                             Diagnostics &errors);

// Validates a stylesheet document against stylesheet.schema.json before it is
// parsed.
bool validate_against_stylesheet_schema(const nlohmann::json &document,
                                        Diagnostics &errors);
} // namespace nZucchini
