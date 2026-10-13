#pragma once

#include "IUserTags.h"

#include <set>
#include <string>

namespace nUserTags {
enum class UserRequirement { IntroducedBeforeReference };
struct UserReferenceRule : DSLRule<UserRequirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"User references", "References must follow a user introduction"};
  }
  Requirements requirements() const override {
    return {{UserRequirement::IntroducedBeforeReference,
             {"Introduced before reference",
              "Every referenced user must have been introduced earlier in the "
              "scenario."}}};
  }
  void
  validate(const StepContext &context,
           nZucchini::ViolationReporter<UserRequirement> &reporter) override {
    const auto &step = context.current().step;
    if (context.tags(step).contains(StepTag::IntroducesUser)) {

      switch (context.cast<StepTag::IntroducesUser>(context.method(step))) {

      case IntroducesUserSteps::introduceUsers: {
        const auto args = context.getArgs<StepMethod::introduceUsers>(step);
        for (const auto &user : args.rows) {
          introducedUsers.insert(user.name);
        }
        break;
      }
      }
    }

    if (context.tags(step).contains(StepTag::ReferencesUser)) {

      switch (context.cast<StepTag::ReferencesUser>(context.method(step))) {

      case ReferencesUserSteps::referenceUser: {
        const auto args = context.getArgs<StepMethod::referenceUser>(step);
        if (introducedUsers.count(args.name) == 0) {
          reporter.violation(
              UserRequirement::IntroducedBeforeReference,
              nZucchini::DiagnosticSeverity::Error,
              "referenced user '" + args.name + "' was not introduced", step);
        }
        break;
      }
      }
    }
  }

private:
  std::set<std::string> introducedUsers;
};
class UserTags : public IUserTagsDefaultThrowing {
public:
  void introduceUsers(const std::vector<User> &) override {}

  void referenceUser(const std::string &) override {}

  void register_rules(RuleRegistry &registry) override {
    registry.add<UserReferenceRule>();
  }
};
} // namespace nUserTags
