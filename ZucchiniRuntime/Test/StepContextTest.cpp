#include <Zucchini/Runtime/ScenarioContext.hpp>
#include <Zucchini/Runtime/StepContext.hpp>
#include <Zucchini/Runtime/StepView.hpp>
#include <Zucchini/Runtime/TagSet.hpp>

#include <gtest/gtest.h>

#include <bitset>
#include <type_traits>
#include <utility>

namespace nStepContextTest {
enum class Method { withArgs, withOtherArgs, noArgs };
enum class Tag { IntroducesUser, ReferencesUser };

using Tags = nZucchini::TagSet<Tag, 2>;

Method resolve(const nZucchini::ZucchiniStep &step) {
  if (step.methodName == "withArgs") {
    return Method::withArgs;
  }
  if (step.methodName == "withOtherArgs") {
    return Method::withOtherArgs;
  }
  return Method::noArgs;
}

Tags resolve_tags(const nZucchini::ZucchiniStep &step) {
  std::bitset<2> bits;
  if (step.methodName == "withArgs") {
    bits.set(static_cast<std::size_t>(Tag::IntroducesUser));
  }
  if (step.methodName == "withOtherArgs") {
    bits.set(static_cast<std::size_t>(Tag::ReferencesUser));
  }
  return Tags(bits);
}

struct Args {
  explicit Args(const nZucchini::ZucchiniStep &step)
      : value(step.captures.at(0).value.get<long>()) {}

  long value;
};

struct OtherArgs {
  explicit OtherArgs(const nZucchini::ZucchiniStep &step)
      : value(step.captures.at(0).value.get<long>() * 2) {}

  long value;
};
} // namespace nStepContextTest

namespace nZucchini {
template <>
struct StepTypeDescriptor<nStepContextTest::Method,
                          nStepContextTest::Method::withArgs> {
  using ArgsType = nStepContextTest::Args;
};

template <>
struct StepTypeDescriptor<nStepContextTest::Method,
                          nStepContextTest::Method::withOtherArgs> {
  using ArgsType = nStepContextTest::OtherArgs;
};
} // namespace nZucchini

namespace {
template <typename View, nStepContextTest::Method Method, typename = void>
struct HasArgs : std::false_type {};

template <typename View, nStepContextTest::Method Method>
struct HasArgs<View, Method,
                std::void_t<decltype(std::declval<const View &>()
                                         .template getArgs<Method>())>>
    : std::true_type {};

template <typename Context, nStepContextTest::Method Method, typename = void>
struct HasContextArgs : std::false_type {};

template <typename Context, nStepContextTest::Method Method>
struct HasContextArgs<
  Context, Method,
  std::void_t<decltype(std::declval<const Context &>()
               .template getArgs<Method>(
                 std::declval<const nZucchini::ZucchiniStep &>()))>>
  : std::true_type {};

using View = nZucchini::StepView<nStepContextTest::Method,
                                nStepContextTest::resolve>;
using Context = nZucchini::ScenarioContext<nStepContextTest::Method,
                                           nStepContextTest::resolve,
                                           nStepContextTest::Tags,
                                           nStepContextTest::resolve_tags>;
using StepContext = nZucchini::StepContext<nStepContextTest::Method,
                                           nStepContextTest::resolve,
                                           nStepContextTest::Tags,
                                           nStepContextTest::resolve_tags>;

static_assert(HasArgs<View, nStepContextTest::Method::withArgs>::value);
static_assert(!HasArgs<View, nStepContextTest::Method::noArgs>::value);
static_assert(HasContextArgs<Context, nStepContextTest::Method::withArgs>::value);
static_assert(!HasContextArgs<Context, nStepContextTest::Method::noArgs>::value);

TEST(StepContext, ResolvesMethodAndConstructsStepView) {
  const nZucchini::ZucchiniStep step(
      "", "withArgs", "step text", {nZucchini::Capture("value", 17)});
  const View view(step);

  EXPECT_EQ(nStepContextTest::Method::withArgs, view.method);
  EXPECT_EQ(17, view.getArgs<nStepContextTest::Method::withArgs>().value);
}

TEST(ScenarioContext, GetsTypedArgsAndRejectsMethodMismatch) {
  const nZucchini::ZucchiniStep step(
      "", "withArgs", "step text", {nZucchini::Capture("value", 23)});
  const Context context(nZucchini::Zucchini("scenario", "feature", {step}));

  EXPECT_EQ(nStepContextTest::Method::withArgs, context.method(step));
  EXPECT_EQ(23,
            context.getArgs<nStepContextTest::Method::withArgs>(step).value);
  EXPECT_THROW(
      context.getArgs<nStepContextTest::Method::withOtherArgs>(step),
      std::logic_error);
}

TEST(ScenarioContext, ResolvesStepTags) {
  const nZucchini::ZucchiniStep introducingStep("", "withArgs", "introduces");
  const nZucchini::ZucchiniStep referencingStep("", "withOtherArgs",
                                                 "references");
  const Context context(nZucchini::Zucchini(
      "scenario", "feature", {introducingStep, referencingStep}));

  EXPECT_TRUE(context.tags(introducingStep)
                  .contains(nStepContextTest::Tag::IntroducesUser));
  EXPECT_FALSE(context.tags(introducingStep)
                   .contains(nStepContextTest::Tag::ReferencesUser));
  EXPECT_TRUE(context.tags(referencingStep)
                  .contains(nStepContextTest::Tag::ReferencesUser));
}

TEST(TagSet, ChecksMembershipAndRejectsOutOfRangeValues) {
  std::bitset<2> bits;
  bits.set(1);
  const nZucchini::TagSet<nStepContextTest::Tag, 2> tags(bits);

  EXPECT_FALSE(tags.contains(nStepContextTest::Tag::IntroducesUser));
  EXPECT_TRUE(tags.contains(nStepContextTest::Tag::ReferencesUser));
  EXPECT_FALSE(tags.contains(static_cast<nStepContextTest::Tag>(2)));
  EXPECT_FALSE(tags.contains(static_cast<nStepContextTest::Tag>(-1)));
}

TEST(StepContext, InheritsScenarioAndProvidesStepNavigation) {
  const nZucchini::ZucchiniStep previous("", "noArgs", "previous");
  const nZucchini::ZucchiniStep current("", "withArgs", "current",
                                        {nZucchini::Capture("value", 1)});
  const nZucchini::ZucchiniStep next("", "noArgs", "next");
  const nZucchini::Zucchini zucchini("scenario", "feature",
                                    {previous, current, next});
  const StepContext context(zucchini, 1);

  EXPECT_EQ(nStepContextTest::Method::withArgs, context.method(current));
  EXPECT_EQ("current", context.current().step.text);
  ASSERT_TRUE(context.previous());
  EXPECT_EQ("previous", context.previous()->step.text);
  ASSERT_TRUE(context.next());
  EXPECT_EQ("next", context.next()->step.text);
}
} // namespace