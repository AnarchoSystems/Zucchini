#include "Zucchini/Snippets.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace nZucchini
{
    namespace
    {
        struct Capture
        {
            std::string name;
            std::string type;
        };

        bool is_digit(char character)
        {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        }

        void escape_regex(char character, std::string& out)
        {
            static const std::string special = R"(\^$.|?*+()[]{})";
            if (special.find(character) != std::string::npos)
            {
                out.push_back('\\');
            }
            out.push_back(character);
        }

        void append_name(const std::string& text, std::string& name, bool& capitalize)
        {
            for (const auto character : text)
            {
                if (std::isalpha(static_cast<unsigned char>(character)) == 0)
                {
                    capitalize = !name.empty();
                    continue;
                }
                if (capitalize)
                {
                    name.push_back(static_cast<char>(std::toupper(character)));
                }
                else
                {
                    name.push_back(name.empty() ? static_cast<char>(std::tolower(character)) : character);
                }
                capitalize = false;
            }
        }

        // Method names are snake_case; words are separated by '_' instead of being capitalized.
        void append_snake_name(const std::string& text, std::string& name, bool& pendingSeparator)
        {
            for (const auto character : text)
            {
                if (std::isalpha(static_cast<unsigned char>(character)) == 0)
                {
                    if (!name.empty())
                    {
                        pendingSeparator = true;
                    }
                    continue;
                }
                if (pendingSeparator)
                {
                    name.push_back('_');
                }
                name.push_back(static_cast<char>(std::tolower(character)));
                pendingSeparator = false;
            }
        }

        // Numbers and quoted strings become capture groups; everything else is matched literally.
        std::string regex_for(const std::string& stepText,
                              std::vector<Capture>& captures,
                              std::string& name)
        {
            std::string pattern = "^";
            bool pendingSeparator = false;
            for (std::size_t index = 0; index < stepText.size();)
            {
                if (stepText[index] == '"')
                {
                    const auto closing = stepText.find('"', index + 1);
                    if (closing != std::string::npos)
                    {
                        captures.push_back({"arg" + std::to_string(captures.size() + 1), "string"});
                        pattern += R"RX("([^"]*)")RX";
                        index = closing + 1;
                        if (!name.empty())
                        {
                            pendingSeparator = true;
                        }
                        continue;
                    }
                }

                const auto negative = stepText[index] == '-' && index + 1 < stepText.size()
                    && is_digit(stepText[index + 1]);
                if (negative || is_digit(stepText[index]))
                {
                    std::size_t cursor = index + (negative ? 1 : 0);
                    while (cursor < stepText.size() && is_digit(stepText[cursor]))
                    {
                        ++cursor;
                    }
                    auto isDecimal = false;
                    if (cursor + 1 < stepText.size() && stepText[cursor] == '.' && is_digit(stepText[cursor + 1]))
                    {
                        isDecimal = true;
                        ++cursor;
                        while (cursor < stepText.size() && is_digit(stepText[cursor]))
                        {
                            ++cursor;
                        }
                    }

                    captures.push_back({"arg" + std::to_string(captures.size() + 1), isDecimal ? "float" : "int"});
                    pattern += isDecimal ? R"((-?\d+\.\d+))" : R"((-?\d+))";
                    index = cursor;
                    if (!name.empty())
                    {
                        pendingSeparator = true;
                    }
                    continue;
                }

                escape_regex(stepText[index], pattern);
                append_snake_name(std::string(1, stepText[index]), name, pendingSeparator);
                ++index;
            }
            return pattern + '$';
        }

        std::string method_name_of(const std::string& stepText)
        {
            std::vector<Capture> captures;
            std::string name;
            regex_for(stepText, captures, name);
            return name.empty() ? "step" : name;
        }

        // A header sanitized the same way step text is: words become a camelCase identifier.
        std::string identifier_from(const std::string& header)
        {
            std::string name;
            bool capitalize = false;
            append_name(header, name, capitalize);
            return name;
        }

        std::string pascal_case_from_snake(const std::string& snakeCase)
        {
            std::string result;
            bool capitalizeNext = true;
            for (const auto character : snakeCase)
            {
                if (character == '_')
                {
                    capitalizeNext = true;
                    continue;
                }
                result.push_back(capitalizeNext ? static_cast<char>(std::toupper(static_cast<unsigned char>(character)))
                                                : character);
                capitalizeNext = false;
            }
            return result;
        }

        std::string table_type_name(const std::string& stepText)
        {
            return pascal_case_from_snake(method_name_of(stepText)) + "Row";
        }

        std::string yaml_quote(const std::string& text)
        {
            std::string quoted = "\"";
            for (const auto character : text)
            {
                if (character == '"' || character == '\\')
                {
                    quoted.push_back('\\');
                }
                quoted.push_back(character);
            }
            quoted.push_back('"');
            return quoted;
        }

        // Every column starts out able to be int/double/bool; each disconfirming example rules one out.
        // Prefer the narrowest type that still fits every observed value; string is the fallback.
        std::string column_type(const UndefinedTableColumn& column)
        {
            if (column.couldBeInt)
            {
                return "int";
            }
            if (column.couldBeDouble)
            {
                return "float";
            }
            if (column.couldBeBool)
            {
                return "bool";
            }
            return {};
        }

        std::string table_type_snippet(const std::string& typeName, const std::vector<UndefinedTableColumn>& columns)
        {
            std::ostringstream snippet;
            snippet << "  - name: " << typeName << '\n';
            snippet << "    kind: struct\n";
            snippet << "    fields:\n";
            for (const auto& column : columns)
            {
                auto fieldName = identifier_from(column.header);
                if (fieldName.empty())
                {
                    fieldName = "field";
                }
                snippet << "      - name: " << fieldName << '\n';
                if (const auto type = column_type(column); !type.empty())
                {
                    snippet << "        type: " << type << '\n';
                }
                if (fieldName != column.header)
                {
                    snippet << "        header: " << yaml_quote(column.header) << '\n';
                }
                if (column.optional)
                {
                    snippet << "        optional: true\n";
                }
            }
            return snippet.str();
        }
    }

    std::string step_snippet(const UndefinedStep& step)
    {
        std::vector<Capture> captures;
        std::string name;
        const auto pattern = regex_for(step.text, captures, name);
        const auto methodName = name.empty() ? "step" : name;

        std::ostringstream snippet;
        snippet << "  - step: " << pattern << '\n';
        snippet << "    methodName: " << methodName << '\n';

        if (!captures.empty())
        {
            snippet << "    arguments:\n";
            for (const auto& capture : captures)
            {
                snippet << "      - name: " << capture.name << '\n';
                snippet << "        type: " << capture.type << '\n';
            }
        }

        if (step.table)
        {
            snippet << "    dataTable:\n";
            snippet << "      type: " << pascal_case_from_snake(methodName) + "Row" << '\n';
        }

        return snippet.str();
    }

    std::string step_snippets(const std::vector<UndefinedStep>& steps)
    {
        if (steps.empty())
        {
            return {};
        }

        std::ostringstream snippet;

        const auto hasTables = std::any_of(steps.begin(), steps.end(),
                                           [](const UndefinedStep& step) { return step.table.has_value(); });
        if (hasTables)
        {
            snippet << "types:\n";
            for (const auto& step : steps)
            {
                if (step.table)
                {
                    snippet << table_type_snippet(table_type_name(step.text), *step.table);
                }
            }
            snippet << '\n';
        }

        snippet << "steps:\n";
        for (const auto& step : steps)
        {
            snippet << step_snippet(step);
        }
        return snippet.str();
    }
}
