#pragma once
#include <map>
#include <optional>
#include <string>
namespace nTables {
struct Entry {
  long value = 0;
  std::optional<std::string> label;
  long scale = 1;
};
struct LooseEntry {
  long value = 0;
  std::map<std::string, std::string> additionalProperties;
};
} // namespace nTables