#include "Lowering.hpp"

#include <gtest/gtest.h>

namespace nZucchini {
TEST(Lowering, AppliesEnumPrefixToImportedCases) {
  const StepDefinitions manifest(
      {EnumType("QualityClass", "qc_", {EnumCase("IO", {"IO"})}, true,
                 "EQualityClass")},
      {StepDefinition("^nothing$", "nothing")});
  Stylesheet stylesheet;

  const auto fixture = lower(manifest, stylesheet, "Fixture");

  ASSERT_EQ(1u, fixture.enums.size());
  ASSERT_EQ(1u, fixture.enums.front().cases.size());
  EXPECT_EQ("EQualityClass::qc_IO", fixture.enums.front().cases.front().cppName);
}
} // namespace nZucchini
