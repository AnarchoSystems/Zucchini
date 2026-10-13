#pragma once

#include "INamingConventionsDemo.h"

namespace nNamingConventionsDemo {
enum class PeopleRequirement { HasRows };
struct PeopleRowsRule : tDslrule<PeopleRequirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"People rows", "People tables must contain a row"};
  }
  Requirements requirements() const override {
    return {{PeopleRequirement::HasRows,
             {"Has rows", "A people table must contain at least one row."}}};
  }
  void
  validate(const tStepContext &context,
           nZucchini::ViolationReporter<PeopleRequirement> &reporter) override {
    const auto current = context.current();
    if (current.method == EStepMethod::step_peopleExist &&
        current.getArgs<EStepMethod::step_peopleExist>().rows.empty()) {
      reporter.violation(PeopleRequirement::HasRows,
                         nZucchini::DiagnosticSeverity::Error,
                         "people table must contain a row", current.step);
    }
  }
};
class NamingConventionsDemo : public INamingConventionsDemo {
public:
  void step_peopleExist(const std::vector<tPerson> &rows) override {
    people = rows;
    ASSERT_EQ(1u, people.size());
    EXPECT_EQ("fixture", people.front().massAdditionalProperty.at("Origin"));
  }

  void step_peopleCount(long expected) override {
    EXPECT_EQ(expected, static_cast<long>(people.size()));
    ASSERT_EQ(1u, people.size());
    EXPECT_EQ("Ada", people.front().msName);
    EXPECT_EQ(30, people.front().mnAge);
    EXPECT_EQ(EColor::Color_red, people.front().meColor);
    ASSERT_EQ(2u, people.front().maeTag.size());
    EXPECT_FALSE(people.front().mopt_sNickname.has_value());
  }

  void hook_aroundStep(const tStepContext &context,
                       const std::function<void()> &step) override {
    const auto current = context.current();
    EXPECT_EQ(to_string(current.method), current.step.methodName);
    if (context.index == 0) {
      EXPECT_FALSE(context.previous());
    } else {
      EXPECT_EQ(context.previous()->step.methodName,
                context.zucchini.steps.at(context.index - 1).methodName);
    }
    if (context.index + 1 == context.zucchini.steps.size()) {
      EXPECT_FALSE(context.next());
    } else {
      EXPECT_EQ(context.next()->step.methodName,
                context.zucchini.steps.at(context.index + 1).methodName);
    }
    step();
  }

  void hook_registerRules(tRuleRegistry &registry) override {
    registry.add<PeopleRowsRule>();
  }

private:
  std::vector<tPerson> people;
};
} // namespace nNamingConventionsDemo
