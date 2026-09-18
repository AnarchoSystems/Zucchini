#include "Zucchini/Diagnostics.hpp"

#include <sstream>

namespace nZucchini
{
    std::string to_string(const CodingPath& path)
    {
        std::ostringstream stream;
        stream << path;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, const CodingPath& path)
    {
        bool first = true;
        for (const auto& element : path)
        {
            if (const auto* name = std::get_if<std::string>(&element))
            {
                if (!first)
                {
                    stream << '.';
                }
                stream << *name;
            }
            else
            {
                stream << '[' << std::get<std::size_t>(element) << ']';
            }
            first = false;
        }
        return stream;
    }

    bool operator==(const Diagnostic& lhs, const Diagnostic& rhs)
    {
        return lhs.path == rhs.path && lhs.message == rhs.message && lhs.line == rhs.line
            && lhs.column == rhs.column;
    }

    std::string to_string(const Diagnostic& diagnostic)
    {
        std::ostringstream stream;
        stream << diagnostic;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, const Diagnostic& diagnostic)
    {
        stream << (diagnostic.path.empty() ? std::string("<document>") : to_string(diagnostic.path));
        if (diagnostic.line)
        {
            stream << " (line " << *diagnostic.line;
            if (diagnostic.column)
            {
                stream << ", column " << *diagnostic.column;
            }
            stream << ')';
        }
        return stream << ": " << diagnostic.message;
    }

    std::string to_string(const Diagnostics& diagnostics)
    {
        std::ostringstream stream;
        for (const auto& diagnostic : diagnostics)
        {
            stream << "\n  " << diagnostic;
        }
        return stream.str();
    }

    std::vector<CodingPath> paths_of(const Diagnostics& diagnostics)
    {
        std::vector<CodingPath> paths;
        paths.reserve(diagnostics.size());
        for (const auto& diagnostic : diagnostics)
        {
            paths.push_back(diagnostic.path);
        }
        return paths;
    }

    void add_diagnostic(Diagnostics& diagnostics, CodingPath path, std::string message)
    {
        diagnostics.push_back(Diagnostic{std::move(path), std::move(message), std::nullopt, std::nullopt});
    }

    void add_diagnostic(Diagnostics& diagnostics,
                        CodingPath path,
                        std::string message,
                        std::uint32_t line,
                        std::uint32_t column)
    {
        diagnostics.push_back(Diagnostic{std::move(path), std::move(message), line, column});
    }
}
