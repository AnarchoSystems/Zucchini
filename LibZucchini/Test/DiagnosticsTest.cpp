#include <Zucchini/Diagnostics.hpp>

#include <gtest/gtest.h>

namespace nZucchini {
TEST(Diagnostics, RendersCompilerStyleLocation) {
  const Diagnostic diagnostic{"example.feature",
                              "doc-string steps must start after a setup step",
                              7, 5};

  EXPECT_EQ("example.feature:7:5: error: doc-string steps must start after a "
            "setup step",
            to_string(diagnostic));
}

TEST(Diagnostics, TracksSeverityAndPrintsNonErrors) {
  Diagnostics diagnostics;
  add_diagnostic(diagnostics, "example.feature", "this may need attention",
                 DiagnosticSeverity::Warning);

  EXPECT_FALSE(has_errors(diagnostics));
  EXPECT_EQ("example.feature: warning: this may need attention",
            to_string(diagnostics.front()));

  add_diagnostic(diagnostics, "example.feature", "this is invalid");
  EXPECT_TRUE(has_errors(diagnostics));
}
} // namespace nZucchini