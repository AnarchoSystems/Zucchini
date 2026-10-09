#pragma once
#include <string>
namespace nZucchini {
// Invokes the generator to validate and insert suggestions into the source YAML.
bool update_step_definitions(const std::string& generator,
                             const std::string& path,
                             const std::string& suggestions,
                             const std::string& workDirectory,
                             std::string& error);
std::string step_definitions_url(const std::string& path);
}
