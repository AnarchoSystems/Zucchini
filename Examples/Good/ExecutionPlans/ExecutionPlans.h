#pragma once
#include "IExecutionPlans.h"
namespace nExecutionPlans {
class ExecutionPlans : public IExecutionPlans {
public:
  void value(long value) override { EXPECT_GT(value, 0); }
};
}
