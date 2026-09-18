#pragma once

#include <cstdint>
#include <string>

namespace nZucchini
{
    struct SourceLocation
    {
        SourceLocation() = default;
        SourceLocation(std::string uri, std::uint32_t line, std::uint32_t column, std::string stepText = {})
            : uri(std::move(uri))
            , line(line)
            , column(column)
            , stepText(std::move(stepText))
        {
        }

        std::string uri;
        std::uint32_t line = 0;
        std::uint32_t column = 0;
        std::string stepText;
    };

    // Set by runScenario before each step so failures can point back at the Gherkin source.
    void set_current_source_location(SourceLocation location);
    void clear_current_source_location();
    const SourceLocation* current_source_location();

    // "<uri>:<line>:<column>", the form VS Code's terminal turns into a link.
    std::string to_string(const SourceLocation& location);
}
