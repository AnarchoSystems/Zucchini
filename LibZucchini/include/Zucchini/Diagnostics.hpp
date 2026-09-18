#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace nZucchini
{
    // One hop into a document: a mapping key or a sequence index.
    using CodingKey = std::variant<std::string, std::size_t>;
    using CodingPath = std::vector<CodingKey>;

    inline CodingKey coding_key(std::string name)
    {
        return CodingKey{std::move(name)};
    }

    inline CodingKey coding_index(std::size_t index)
    {
        return CodingKey{index};
    }

    // Renders a path as "steps[1].arguments[0].type".
    std::string to_string(const CodingPath &path);
    std::ostream &operator<<(std::ostream &stream, const CodingPath &path);

    struct Diagnostic
    {
        Diagnostic() = default;
        Diagnostic(CodingPath path,
                   std::string message,
                   std::optional<std::uint32_t> line = std::nullopt,
                   std::optional<std::uint32_t> column = std::nullopt)
            : path(std::move(path))
            , message(std::move(message))
            , line(line)
            , column(column)
        {
        }

        CodingPath path;
        std::string message;
        std::optional<std::uint32_t> line;
        std::optional<std::uint32_t> column;
    };

    using Diagnostics = std::vector<Diagnostic>;

    bool operator==(const Diagnostic &lhs, const Diagnostic &rhs);
    std::string to_string(const Diagnostic &diagnostic);
    std::ostream &operator<<(std::ostream &stream, const Diagnostic &diagnostic);

    std::string to_string(const Diagnostics &diagnostics);
    std::vector<CodingPath> paths_of(const Diagnostics &diagnostics);

    void add_diagnostic(Diagnostics &diagnostics, CodingPath path, std::string message);
    void add_diagnostic(Diagnostics &diagnostics,
                        CodingPath path,
                        std::string message,
                        std::uint32_t line,
                        std::uint32_t column);
}
