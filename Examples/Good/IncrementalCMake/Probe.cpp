#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  std::filesystem::path feature_dir;
  std::filesystem::path manifest_dir;
  bool list_tests = false;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--gtest_list_tests") {
      list_tests = true;
    } else if (argument.rfind("feature_dir=", 0) == 0) {
      feature_dir = argument.substr(std::string("feature_dir=").size());
    } else if (argument.rfind("manifest_dir=", 0) == 0) {
      manifest_dir = argument.substr(std::string("manifest_dir=").size());
    }
  }

  if (!list_tests) {
    return 0;
  }
  if (feature_dir.empty() || manifest_dir.empty()) {
    std::cerr << "Discovery requires feature_dir and manifest_dir\n";
    return 1;
  }

  std::filesystem::create_directories(manifest_dir);
  std::ofstream marker(manifest_dir / "discovery.marker", std::ios::app);
  if (!marker) {
    std::cerr << "Could not open discovery marker\n";
    return 1;
  }
  marker << "x";
  std::cout << "Probe.\n";
  for (const auto &entry : std::filesystem::directory_iterator(feature_dir)) {
    if (entry.path().extension() != ".feature") {
      continue;
    }
    std::ifstream feature(entry.path());
    if (!feature) {
      std::cerr << "Could not open feature " << entry.path() << '\n';
      return 1;
    }
    std::string line;
    while (std::getline(feature, line)) {
      const auto scenario = line.find("Scenario: ");
      if (scenario != std::string::npos) {
        std::cout << "  " << line.substr(scenario + std::string("Scenario: ").size()) << '\n';
      }
    }
  }
  return 0;
}