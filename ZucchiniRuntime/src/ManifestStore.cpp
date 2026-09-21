#include "ManifestStore.hpp"

#include "Naming.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace nZucchini {
std::string manifest_path(const std::string &directory,
                          const std::string &testName) {
  return (std::filesystem::path(directory) / (testName + ".json")).string();
}

bool store_zucchini(const std::string &directory, const Zucchini &zucchini,
                    Diagnostics &errors) {
  std::error_code failure;
  std::filesystem::create_directories(directory, failure);

  const auto path = manifest_path(directory, test_name(zucchini));
  std::ofstream file(path);
  if (!file) {
    add_diagnostic(errors, path, "cannot write zucchini manifest");
    return false;
  }

  file << nlohmann::json(zucchini).dump(2) << '\n';
  return true;
}

bool store_zucchinis(const std::string &directory,
                     const std::vector<Zucchini> &zucchinis,
                     Diagnostics &errors) {
  bool stored = true;
  for (const auto &zucchini : zucchinis) {
    stored = store_zucchini(directory, zucchini, errors) && stored;
  }
  return stored;
}

bool load_zucchini(const std::string &directory, const std::string &testName,
                   Zucchini &zucchini, Diagnostics &errors) {
  const auto path = manifest_path(directory, testName);
  std::ifstream file(path);
  if (!file) {
    add_diagnostic(errors, path, "no zucchini manifest; re-run test discovery");
    return false;
  }

  try {
    zucchini = nlohmann::json::parse(file).get<Zucchini>();
  } catch (const nlohmann::json::exception &failure) {
    add_diagnostic(errors, path, failure.what());
    return false;
  }
  return true;
}

bool load_zucchinis(const std::string &directory,
                    std::vector<Zucchini> &zucchinis, Diagnostics &errors) {
  zucchinis.clear();

  std::error_code failure;
  if (!std::filesystem::is_directory(directory, failure)) {
    add_diagnostic(errors, directory,
                   "no manifest directory; re-run test discovery");
    return false;
  }

  std::vector<std::string> names;
  for (const auto &entry :
       std::filesystem::directory_iterator(directory, failure)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      names.push_back(entry.path().stem().string());
    }
  }
  std::sort(names.begin(), names.end());
  zucchinis.reserve(names.size());

  for (const auto &name : names) {
    Zucchini zucchini;
    if (!load_zucchini(directory, name, zucchini, errors)) {
      zucchinis.clear();
      return false;
    }
    zucchinis.push_back(std::move(zucchini));
  }

  return true;
}
} // namespace nZucchini
