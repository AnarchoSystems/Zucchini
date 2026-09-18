#pragma once

#include <string>
#include <vector>

namespace nZucchini
{
    // A ready-to-paste step definition for a step the manifest does not cover.
    std::string step_snippet(const std::string& stepText);

    // The same, as a "steps:" block covering every undefined step.
    std::string step_snippets(const std::vector<std::string>& stepTexts);
}
