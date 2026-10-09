#pragma once
#include <string>
namespace nZucchini {
// Inserts missing suggestions without reserializing existing source text.
void update_manifest(const std::string& path, const std::string& suggestionsPath);
}
