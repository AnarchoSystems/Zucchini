#pragma once

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/StepDefinitions.hpp>

#include <string>

namespace nZucchini {
bool parse_step_def_manifest(const std::string &yaml,
                             StepDefinitions &definitions,
                             Diagnostics &errors);
bool parse_step_def_manifest_from_file(const std::string &filePath,
                                       StepDefinitions &definitions,
                                       Diagnostics &errors);
} // namespace nZucchini
