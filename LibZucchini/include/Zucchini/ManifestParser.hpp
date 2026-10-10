#pragma once

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/StepDefinitions.hpp>

#include <string>
#include <vector>

namespace nZucchini {
struct ParsedManifest {
    StepDefinitions definitions;
    std::vector<std::vector<std::string>> stepTags;
};

bool parse_step_def_manifest(const std::string &yaml,
                                                         ParsedManifest &manifest,
                             Diagnostics &errors);
bool parse_step_def_manifest_from_file(const std::string &filePath,
                                                                             ParsedManifest &manifest,
                                       Diagnostics &errors);
} // namespace nZucchini
