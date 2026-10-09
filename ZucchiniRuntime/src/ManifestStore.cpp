#include "ManifestStore.hpp"

#include "Naming.hpp"

#include <algorithm>
#include <chrono>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include <fstream>

namespace nZucchini {
std::string manifest_path(const std::string &directory,
                          const std::string &testName) {
  return (std::filesystem::path(directory) / (testName + ".json")).string();
}

bool list_manifest_names(const std::string &directory,
                          std::vector<std::string> &names, Diagnostics &errors) {
  names.clear();
  std::error_code failure;
  if (!std::filesystem::is_directory(directory, failure)) {
    add_diagnostic(errors, directory, failure
        ? "cannot inspect manifest directory: " + failure.message()
        : "no manifest directory; re-run test discovery");
    return false;
  }
  std::filesystem::directory_iterator iterator(directory, failure), end;
  while (!failure && iterator != end) {
    const auto &entry = *iterator;
    const auto regular = entry.is_regular_file(failure);
    if (failure) {
      add_diagnostic(errors, entry.path().string(),
                     "cannot inspect manifest entry: " + failure.message());
      names.clear();
      return false;
    }
    if (regular && entry.path().extension() == ".json") {
      names.push_back(entry.path().stem().string());
    }
    iterator.increment(failure);
  }
  if (failure) {
    add_diagnostic(errors, directory, "cannot list manifests: " + failure.message());
    names.clear();
    return false;
  }
  std::sort(names.begin(), names.end());
  return true;
}

bool store_zucchini(const std::string &directory, const Zucchini &zucchini,
                    Diagnostics &errors) {
  std::error_code failure;
  std::filesystem::create_directories(directory, failure);
  if (failure) {
    add_diagnostic(errors, directory, "cannot create manifest directory: " + failure.message());
    return false;
  }
  const auto path = manifest_path(directory, test_name(zucchini));
  const auto temporary = path + ".tmp-" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  try {
    const auto serialized = nlohmann::json(zucchini).dump(2);
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file << serialized << '\n';
    file.close();
    if (!file) {
      throw std::runtime_error("cannot write zucchini manifest");
    }
#ifdef _WIN32
    if (!MoveFileExW(std::filesystem::path(temporary).c_str(),
                     std::filesystem::path(path).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      throw std::runtime_error("cannot replace zucchini manifest");
    }
#else
    std::filesystem::rename(temporary, path);
#endif
  } catch (const std::exception &error) {
    std::filesystem::remove(temporary, failure);
    add_diagnostic(errors, path, error.what());
    return false;
  }
  return true;
}

bool store_zucchinis(const std::string &directory,
                     const std::vector<Zucchini> &zucchinis,
                     Diagnostics &errors) {
  // Discovery publishes a complete set, including an empty set. Never remove
  // previous plans when writing the new set fails.
  std::error_code failure;
  std::filesystem::create_directories(directory, failure);
  if (failure) {
    add_diagnostic(errors, directory, "cannot create manifest directory: " + failure.message());
    return false;
  }
  std::vector<std::string> oldNames;
  if (!list_manifest_names(directory, oldNames, errors)) return false;
  std::set<std::string> names;
  for (const auto &zucchini : zucchinis) {
    const auto name = test_name(zucchini);
    if (!names.insert(name).second) {
      add_diagnostic(errors, directory, "duplicate execution plan name '" + name + "'");
      return false;
    }
  }
  for (const auto &zucchini : zucchinis) {
    if (!store_zucchini(directory, zucchini, errors)) return false;
  }
  for (const auto &name : oldNames) {
    if (names.count(name)) continue;
    std::filesystem::remove(manifest_path(directory, name), failure);
    if (failure) {
      add_diagnostic(errors, manifest_path(directory, name),
                     "cannot remove obsolete manifest: " + failure.message());
      return false;
    }
  }
  return true;
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
    if (file.bad()) {
      add_diagnostic(errors, path, "cannot read zucchini manifest");
      return false;
    }
  } catch (const nlohmann::json::exception &failure) {
    add_diagnostic(errors, path, failure.what());
    return false;
  }
  return true;
}

bool load_zucchinis(const std::string &directory,
                    std::vector<Zucchini> &zucchinis, Diagnostics &errors) {
  zucchinis.clear();

  std::vector<std::string> names;
  if (!list_manifest_names(directory, names, errors)) return false;
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
