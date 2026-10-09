#include <Zucchini/Runtime/ExecutionPlan.hpp>
#include <filesystem>
#include <iostream>

// Exercise the public storage boundary with invalid plans, independent of
// discovery's existing name deduplication.
int main(int argc, char** argv) {
  if (argc != 2) return 1;
  const std::string directory = argv[1];
  std::filesystem::remove_all(directory);
  const nZucchini::Zucchini original("scenario", "feature");
  nZucchini::Diagnostics errors;
  if (!nZucchini::store_execution_plans(directory, {original}, errors)) return 1;
  auto duplicate = original;
  duplicate.uri = "would overwrite original";
  if (nZucchini::store_execution_plans(directory, {original, duplicate}, errors) ||
      nZucchini::to_string(errors).find("duplicate execution plan name") == std::string::npos) return 1;
  errors.clear();
  auto invalid = original;
  invalid.uri = std::string(1, static_cast<char>(0xff));
  if (nZucchini::store_execution_plans(directory, {invalid}, errors) || errors.empty()) return 1;
  errors.clear();
  std::vector<nZucchini::Zucchini> loaded;
  if (!nZucchini::load_execution_plans(directory, loaded, errors) ||
      loaded.size() != 1 || !(loaded.front() == original)) return 1;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.path().extension() != ".json") return 1;
  }
  std::cout << "Rejected duplicate and unserializable plans; original plan preserved.\n";
  return 0;
}
