#pragma once

#include <Zucchini/Diagnostics.hpp>

#include <nlohmann/json.hpp>

namespace nZucchini {
// Validates a manifest document against schema.json before it is parsed.
bool validate_against_schema(const nlohmann::json &document,
                             Diagnostics &errors);

bool validate_document_against_schema(const nlohmann::json &document,
                                     const nlohmann::json &schema, Diagnostics &errors);
} // namespace nZucchini
