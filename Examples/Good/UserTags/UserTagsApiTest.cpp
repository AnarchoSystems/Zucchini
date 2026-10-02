#include <gtest/gtest.h>

#include "IUserTags.h"

namespace nUserTags {
TEST(UserTagsApi, CastsMembersAndRejectsNonmembers) {
  EXPECT_EQ(IntroducesUserSteps::introduceUsers,
            cast<StepTag::IntroducesUser>(StepMethod::introduceUsers));
  const ScenarioContext context(nZucchini::Zucchini("scenario", "feature", {}));
  EXPECT_EQ(IntroducesUserSteps::introduceUsers,
            context.cast<StepTag::IntroducesUser>(StepMethod::introduceUsers));
  EXPECT_THROW(context.cast<StepTag::IntroducesUser>(StepMethod::referenceUser),
               std::logic_error);
}
} // namespace nUserTags