#include "Zucchini/Runtime/ManifestUpdate.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <process.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

namespace nZucchini {
namespace {
int run_generator(const std::string& generator, const std::string& path,
                  const std::string& suggestionsPath) {
  std::vector<std::string> arguments{generator, "-update", path, "-suggestions", suggestionsPath};
  std::vector<char*> argv;
  for (auto& argument : arguments) {
    argv.push_back(argument.data());
  }
  argv.push_back(nullptr);
#ifdef _WIN32
  return static_cast<int>(_spawnv(_P_WAIT, generator.c_str(), argv.data()));
#else
  pid_t child = 0;
  if (posix_spawn(&child, generator.c_str(), nullptr, nullptr, argv.data(), environ) != 0) {
    return -1;
  }
  int status = 0;
  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) {
      return -1;
    }
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}
}

bool update_step_definitions(const std::string& generator, const std::string& path,
                             const std::string& suggestions,
                             const std::string& workDirectory,
                             std::string& error) {
  std::filesystem::path temporary;
  try {
    std::filesystem::create_directories(workDirectory);
    temporary = std::filesystem::path(workDirectory) /
        ("step-suggestions-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml");
    {
      std::ofstream file(temporary, std::ios::binary);
      file << suggestions;
      file.close();
      if (!file) {
        throw std::runtime_error("cannot write step suggestions");
      }
    }
    const auto result = run_generator(generator, path, temporary.string());
    std::filesystem::remove(temporary);
    if (result != 0) {
      error = "generator could not update step definitions";
      return false;
    }
    return true;
  } catch (const std::exception& failure) {
    std::error_code ignored;
    if (!temporary.empty()) {
      std::filesystem::remove(temporary, ignored);
    }
    error = failure.what();
    return false;
  }
}
std::string step_definitions_url(const std::string& path) {
  auto absolute = std::filesystem::absolute(path).lexically_normal().generic_string();
  if (absolute.front() != '/') absolute.insert(absolute.begin(), '/');
  std::string result = "vscode://file";
  constexpr char hex[] = "0123456789ABCDEF";
  for (const unsigned char character : absolute) {
    if ((character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') ||
        character == '/' || character == ':' || character == '-' ||
        character == '_' || character == '.' || character == '~') {
      result.push_back(static_cast<char>(character));
    } else {
      result += '%';
      result += hex[character >> 4];
      result += hex[character & 15];
    }
  }
  return result + ":1:1";
}
}
