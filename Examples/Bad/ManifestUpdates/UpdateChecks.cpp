#include <Zucchini/Runtime/ManifestUpdate.hpp>
#include <gtest/gtest.h>
#include <cstdlib>
#include <fstream>
#include <sstream>

TEST(ManifestEditing, Update) {
  const auto* path = std::getenv("MANIFEST_UPDATE_PATH");
  const auto* suggestions = std::getenv("MANIFEST_SUGGESTIONS_PATH");
  if (!path || !suggestions) GTEST_SKIP() << "Run through the manifest editing harness";
  std::ifstream file(suggestions, std::ios::binary);
  ASSERT_TRUE(file);
  std::ostringstream text;
  text << file.rdbuf();
  std::string error;
  ASSERT_TRUE(nZucchini::update_step_definitions(path, text.str(), error)) << error;
}
