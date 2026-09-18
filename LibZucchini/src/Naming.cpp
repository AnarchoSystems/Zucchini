#include "Zucchini/Naming.hpp"

#include <algorithm>
#include <map>

namespace nZucchini
{
    namespace
    {
        // Latin-1/Latin Extended-A characters spelled out so gtest names stay ASCII.
        const std::map<std::string, std::string>& transliterations()
        {
            static const std::map<std::string, std::string> table = {
                {"\u00c4", "Ae"}, {"\u00d6", "Oe"}, {"\u00dc", "Ue"}, {"\u00e4", "ae"}, {"\u00f6", "oe"},
                {"\u00fc", "ue"}, {"\u00df", "ss"}, {"\u00c6", "Ae"}, {"\u00e6", "ae"}, {"\u0152", "Oe"},
                {"\u0153", "oe"}, {"\u00d8", "O"},  {"\u00f8", "o"},  {"\u00c5", "A"},  {"\u00e5", "a"},
                {"\u00c0", "A"},  {"\u00c1", "A"},  {"\u00c2", "A"},  {"\u00c3", "A"},  {"\u00e0", "a"},
                {"\u00e1", "a"},  {"\u00e2", "a"},  {"\u00e3", "a"},  {"\u00c7", "C"},  {"\u00e7", "c"},
                {"\u00c8", "E"},  {"\u00c9", "E"},  {"\u00ca", "E"},  {"\u00cb", "E"},  {"\u00e8", "e"},
                {"\u00e9", "e"},  {"\u00ea", "e"},  {"\u00eb", "e"},  {"\u00cc", "I"},  {"\u00cd", "I"},
                {"\u00ce", "I"},  {"\u00cf", "I"},  {"\u00ec", "i"},  {"\u00ed", "i"},  {"\u00ee", "i"},
                {"\u00ef", "i"},  {"\u00d1", "N"},  {"\u00f1", "n"},  {"\u00d2", "O"},  {"\u00d3", "O"},
                {"\u00d4", "O"},  {"\u00d5", "O"},  {"\u00f2", "o"},  {"\u00f3", "o"},  {"\u00f4", "o"},
                {"\u00f5", "o"},  {"\u00d9", "U"},  {"\u00da", "U"},  {"\u00db", "U"},  {"\u00f9", "u"},
                {"\u00fa", "u"},  {"\u00fb", "u"},  {"\u00dd", "Y"},  {"\u00fd", "y"},  {"\u00ff", "y"},
            };
            return table;
        }

        bool is_ascii_alnum(unsigned char character)
        {
            return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z');
        }

        std::size_t utf8_length(unsigned char lead)
        {
            if ((lead & 0xE0u) == 0xC0u)
            {
                return 2;
            }
            if ((lead & 0xF0u) == 0xE0u)
            {
                return 3;
            }
            if ((lead & 0xF8u) == 0xF0u)
            {
                return 4;
            }
            return 1;
        }

        void append_separator(std::string& out)
        {
            if (!out.empty() && out.back() != '_')
            {
                out.push_back('_');
            }
        }
    }

    std::string sanitize_name(const std::string& text)
    {
        std::string out;
        out.reserve(text.size());

        for (std::size_t index = 0; index < text.size();)
        {
            const auto lead = static_cast<unsigned char>(text[index]);
            if (lead < 0x80u)
            {
                if (is_ascii_alnum(lead))
                {
                    out.push_back(text[index]);
                }
                else
                {
                    append_separator(out);
                }
                ++index;
                continue;
            }

            const auto length = std::min(utf8_length(lead), text.size() - index);
            const auto character = text.substr(index, length);
            index += length;

            const auto replacement = transliterations().find(character);
            if (replacement != transliterations().end())
            {
                out += replacement->second;
            }
            else
            {
                append_separator(out);
            }
        }

        while (!out.empty() && out.back() == '_')
        {
            out.pop_back();
        }

        return out.empty() ? std::string("Unnamed") : out;
    }

    std::string test_name(const std::string& feature, const std::string& rule, const std::string& scenario)
    {
        std::string name = sanitize_name(feature);
        if (!rule.empty())
        {
            name += "__" + sanitize_name(rule);
        }
        return name + "__" + sanitize_name(scenario);
    }

    std::string test_name(const Zucchini& zucchini)
    {
        return test_name(zucchini.featureName, zucchini.ruleName, zucchini.name);
    }

    std::string display_name(const std::string& feature, const std::string& rule, const std::string& scenario)
    {
        std::string name = feature;
        if (!rule.empty())
        {
            name += ": " + rule;
        }
        return name + ": " + scenario;
    }

    std::string display_name(const Zucchini& zucchini)
    {
        return display_name(zucchini.featureName, zucchini.ruleName, zucchini.name);
    }
}
