#include <Zucchini/Runtime/ScenarioContext.hpp>
#include <Zucchini/Runtime/StepContext.hpp>
#include <Zucchini/Runtime/StepView.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace nStepContextTest {
enum class Method { withArgs, withOtherArgs, noArgs };

Method resolve(const nZucchini::ZucchiniStep &step) {
  if (step.methodName == "withArgs") {
    return Method::withArgs;
  }
  if (step.methodName == "withOtherArgs") {
    return Method::withOtherArgs;
  }
  return Method::noArgs;
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
                                           nStepContextTest::resolve>;
using StepContext = nZucchini::StepContext<nStepContextTest::Method,
                                           nStepContextTest::resolve>;

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