#include "Zucchini/Snippets.hpp"

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

        // Numbers and quoted strings become capture groups; everything else is matched literally.
        std::string regex_for(const std::string& stepText,
                              std::vector<Capture>& captures,
                              std::string& name)
        {
            std::string pattern = "^";
            bool capitalize = false;
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
                        capitalize = !name.empty();
                        continue;
                    }
                }

                const auto negative = stepText[index] == '-' && index + 1 < stepText.size()
                    && is_digit(stepText[index + 1]);
                if (negative || is_digit(stepText[index]))
                {
                    captures.push_back({"arg" + std::to_string(captures.size() + 1), "int"});
                    pattern += R"((-?\d+))";
                    index += negative ? 1 : 0;
                    while (index < stepText.size() && is_digit(stepText[index]))
                    {
                        ++index;
                    }
                    capitalize = !name.empty();
                    continue;
                }

                escape_regex(stepText[index], pattern);
                append_name(std::string(1, stepText[index]), name, capitalize);
                ++index;
            }
            return pattern + '$';
        }
    }

    std::string step_snippet(const std::string& stepText)
    {
        std::vector<Capture> captures;
        std::string name;
        const auto pattern = regex_for(stepText, captures, name);

        std::ostringstream snippet;
        snippet << "  - step: " << pattern << '\n';
        snippet << "    methodName: " << (name.empty() ? "step" : name) << '\n';

        if (!captures.empty())
        {
            snippet << "    arguments:\n";
            for (const auto& capture : captures)
            {
                snippet << "      - name: " << capture.name << '\n';
                snippet << "        type: " << capture.type << '\n';
            }
        }

        return snippet.str();
    }

    std::string step_snippets(const std::vector<std::string>& stepTexts)
    {
        if (stepTexts.empty())
        {
            return {};
        }

        std::ostringstream snippet;
        snippet << "steps:\n";
        for (const auto& stepText : stepTexts)
        {
            snippet << step_snippet(stepText);
        }
        return snippet.str();
    }
}
