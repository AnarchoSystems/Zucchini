#pragma once

#include <optional>
#include <string>
#include <utility>

namespace nZucchini {
// Paths passed by CMake to the generated executable during discovery/runtime.
struct DiscoveryArgs {
  std::string featureDir;
  std::string manifestDir;
};

inline DiscoveryArgs parse_discovery_args(int argc, char **argv) {
  DiscoveryArgs args;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    const auto read_value = [&](const std::string &key) -> std::optional<std::string> {
      const auto prefix = key + '=';
      if (argument.rfind(prefix, 0) == 0) {
        return argument.substr(prefix.size());
      }
      return std::nullopt;
    };
    if (const auto featureDir = read_value("feature_dir")) {
      args.featureDir = *featureDir;
    } else if (const auto manifestDir = read_value("manifest_dir")) {
      args.manifestDir = *manifestDir;
    }
  }
  return args;
}
} // namespace nZucchini
