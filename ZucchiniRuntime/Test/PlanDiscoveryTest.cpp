#include <Zucchini/Runtime/PlanDiscovery.hpp>
#include <Zucchini/Runtime/SourceLocation.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>

namespace {
using namespace nZucchini;

TEST(SourceLocation, PrintsCurrentStepForFailures) {
  std::ostringstream stream;
  print_current_source_location(stream);
  EXPECT_EQ("", stream.str());

  set_current_source_location(
      SourceLocation("C:/features with spaces/Shop.feature", 12, 5, "Given a shop"));
  print_current_source_location(stream);
  clear_current_source_location();
  EXPECT_EQ("gherkin source:\nC:/features with spaces/Shop.feature:12:5: "
            "Given a shop\n",
            stream.str());
}

TEST(PlanDiscovery, UnknownPlansRegisterWithoutLocation) {
  const auto directory =
      std::filesystem::temp_directory_path() / "zucchini missing manifests";
  const auto registrations =
      locate_execution_plans(directory.string(), {"Shop.buy"});
  ASSERT_EQ(1u, registrations.size());
  EXPECT_EQ("Shop.buy", registrations[0].name);
  EXPECT_TRUE(registrations[0].location.uri.empty());
  EXPECT_EQ(0u, registrations[0].location.line);
}
} // namespace
