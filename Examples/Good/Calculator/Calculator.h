#pragma once
#include "ICalculator.h"
namespace nCalculator {
class Calculator : public ICalculator {
public:
  ~Calculator() override {
    if (setupCalls != 0) {
      EXPECT_EQ(1, teardownCalls);
    }
  }

  void startWith(long value) override { current = value; }
  void add(long value) override { current += value; }
  void subtract(long value) override { current -= value; }
  void resultIs(long value) override {
    EXPECT_EQ(1, setupCalls);
    EXPECT_EQ(0, teardownCalls);
    EXPECT_EQ(value, current);
  }
  void reset() override { current = 0; }

protected:
  void SetUp() override { ++setupCalls; }
  void TearDown() override { ++teardownCalls; }

private:
  long current = 0;
  int setupCalls = 0;
  int teardownCalls = 0;
};
} // namespace nCalculator
