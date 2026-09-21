#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char **argv) {
  std::filesystem::path output;
  std::string fixture;
  for (int index = 1; index + 1 < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "-o") {
      output = argv[++index];
    } else if (argument == "-fixture") {
      fixture = argv[++index];
    }
  }

  if (output.empty() || fixture.empty()) {
    return 1;
  }

  std::filesystem::create_directories(output);
  std::ofstream(output / ("I" + fixture + ".h")) << "#pragma once\n";
  std::ofstream(output / (fixture + "Test.cc"))
      << "int generated_probe() { return 0; }\n";
  return 0;
}