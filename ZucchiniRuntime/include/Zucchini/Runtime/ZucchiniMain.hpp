#pragma once

#include <gtest/gtest.h>

namespace nZucchini {
// Prints the Gherkin source position of the running step whenever a test part
// fails.
class ZucchiniListener : public testing::EmptyTestEventListener {
public:
  void OnTestPartResult(const testing::TestPartResult &result) override;
};

int ZucchiniMain(int argc, char **argv);
} // namespace nZucchini
