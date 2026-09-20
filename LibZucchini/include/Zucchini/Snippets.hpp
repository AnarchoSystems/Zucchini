#pragma once

#include <optional>
#include <string>
#include <vector>

namespace nZucchini
{
    // A data table column as it appeared in a scenario: the header text as written, and whether
    // it was present in every occurrence of the step or only some (making the field optional).
    struct UndefinedTableColumn
    {
        std::string header;
        bool optional = false;
    };

    // A step with no matching definition in the manifest.
    struct UndefinedStep
    {
        std::string text;
        // Set when at least one occurrence of the step had a data table argument. Columns are the
        // union of headers seen across every occurrence of the step.
        std::optional<std::vector<UndefinedTableColumn>> table;
    };

    // A ready-to-paste step definition for a step the manifest does not cover.
    std::string step_snippet(const UndefinedStep& step);

    // The same, as a "types:"/"steps:" manifest fragment covering every undefined step.
    std::string step_snippets(const std::vector<UndefinedStep>& steps);
}
