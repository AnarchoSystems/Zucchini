#pragma once
#include <string>
namespace nZucchini {
// Validate and insert inferred definitions directly into the source manifest.
bool update_step_definitions(const std::string& path, const std::string& suggestions,
                             std::string& error);
std::string step_definitions_url(const std::string& path);
}
