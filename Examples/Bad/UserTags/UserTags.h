#pragma once

#include "IUserTags.h"

#include <set>
#include <string>

namespace nUserTags {
class UserTags : public IUserTagsDefaultThrowing {
public:
  void introduceUsers(const std::vector<User> &) override {}

  void referenceUser(const std::string &) override {}

  void validate_scenario(const ScenarioContext &context,
                         nZucchini::Diagnostics &errors) override {

    std::set<std::string> introducedUsers;

    for (const auto &step : context.zucchini.steps) {

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
            nZucchini::add_diagnostic(errors, context.zucchini.uri,
                                      "referenced user '" + args.name +
                                          "' was not introduced",
                                      step.line, step.column);
          }
          break;
        }
        }
      }
    }
  }
};
} // namespace nUserTags