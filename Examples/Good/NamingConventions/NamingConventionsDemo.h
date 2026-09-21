#pragma once

#include "INamingConventionsDemo.h"

namespace nNamingConventionsDemo {
class NamingConventionsDemo : public INamingConventionsDemo {
public:
  void peopleExist(const std::vector<tPerson> &rows) override { people = rows; }

  void peopleCount(long expected) override {
    EXPECT_EQ(expected, static_cast<long>(people.size()));
    ASSERT_EQ(1u, people.size());
    EXPECT_EQ("Ada", people.front().msname);
    EXPECT_EQ(30, people.front().miage);
    EXPECT_EQ(EColor::red, people.front().mecolor);
    ASSERT_EQ(2u, people.front().mletags.size());
    EXPECT_FALSE(people.front().mosnickname.has_value());
  }

  void aroundStep(const StepContext &context,
                  const std::function<void()> &step) override {
    step();
    (void)context;
  }

  void validateScenario(const Zucchini &zucchini,
                        const cucumber::messages::pickle &pickle,
                        nZucchini::Diagnostics &errors) override {
    (void)zucchini;
    (void)pickle;
    (void)errors;
  }

private:
  std::vector<tPerson> people;
};
} // namespace nNamingConventionsDemo
