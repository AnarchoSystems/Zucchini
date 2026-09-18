#pragma once

#include "Zucchini/Zucchini.hpp"

#include <string>

namespace nZucchini
{
    // "Feature__Rule__Scenario", sanitised so that gtest accepts it as a test name.
    std::string test_name(const std::string& feature, const std::string& rule, const std::string& scenario);
    std::string test_name(const Zucchini& zucchini);

    // "Feature: Rule: Scenario", what editors show.
    std::string display_name(const std::string& feature, const std::string& rule, const std::string& scenario);
    std::string display_name(const Zucchini& zucchini);

    std::string sanitize_name(const std::string& text);
}
