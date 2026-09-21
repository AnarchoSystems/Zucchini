#pragma once

#include "Zucchini/Runtime/NameCasing.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"

#include <string>

namespace nZucchini {
// Splits `text` into words (on non-alphanumeric boundaries and case
// transitions) and rejoins them per `casing`. Digits stick to the word they
// trail; leading/trailing separators are dropped.
std::string apply_casing(const std::string &text, NameCasing casing);

// "Feature__Rule__Scenario", sanitised so that gtest accepts it as a test name.
std::string test_name(const std::string &feature, const std::string &rule,
                      const std::string &scenario);
std::string test_name(const Zucchini &zucchini);

// "Feature: Rule: Scenario", what editors show.
std::string display_name(const std::string &feature, const std::string &rule,
                         const std::string &scenario);
std::string display_name(const Zucchini &zucchini);

std::string sanitize_name(const std::string &text);
} // namespace nZucchini
