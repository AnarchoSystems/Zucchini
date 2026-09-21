#include "Zucchini/ZucchiniMain.hpp"

#include "Zucchini/SourceLocation.hpp"

#include <iostream>

namespace nZucchini {
void ZucchiniListener::OnTestPartResult(const testing::TestPartResult &result) {
  if (!result.failed()) {
    return;
  }

  const auto *location = current_source_location();
  if (location == nullptr) {
    return;
  }

  std::cout << "gherkin source:\n" << to_string(*location);
  if (!location->stepText.empty()) {
    std::cout << ": " << location->stepText;
  }
  std::cout << std::endl;
}

int ZucchiniMain(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::UnitTest::GetInstance()->listeners().Append(new ZucchiniListener());
  return RUN_ALL_TESTS();
}
} // namespace nZucchini
