#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Runtime/PickleScenario.hpp"
#include "Zucchini/Runtime/Zucchini.hpp"
#include "Zucchini/StepDefinitions.hpp"

#include <string>
#include <vector>

namespace nZucchini {
bool make_execution_plan(const PickleScenario &scenario,
                         const StepDefinitions &definition,
                         Zucchini &plan, Diagnostics &errors);

std::string execution_plan_name(const Zucchini &plan);
bool store_execution_plans(const std::string &directory,
                           const std::vector<Zucchini> &plans,
                           Diagnostics &errors);
bool load_execution_plans(const std::string &directory,
                          std::vector<Zucchini> &plans,
                          Diagnostics &errors);
bool list_execution_plan_names(const std::string &directory,
                               std::vector<std::string> &names,
                               Diagnostics &errors);
bool load_execution_plan(const std::string &directory,
                         const std::string &testName, Zucchini &plan,
                         Diagnostics &errors);
} // namespace nZucchini