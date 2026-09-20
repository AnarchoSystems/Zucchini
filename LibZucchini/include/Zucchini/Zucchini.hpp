#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace nZucchini
{
    // A typed regex sub-expression; the value is a JSON bool, integer or string.
    struct Capture
    {
        Capture() = default;
        Capture(std::string name, nlohmann::json value)
            : name(std::move(name))
            , value(std::move(value))
        {
        }

        std::string name;
        nlohmann::json value;
    };

    struct DocStringArgument
    {
        DocStringArgument() = default;
        explicit DocStringArgument(std::string content, std::optional<std::string> mediaType = std::nullopt)
            : content(std::move(content))
            , mediaType(std::move(mediaType))
        {
        }

        std::string content;
        std::optional<std::string> mediaType;
    };

    // Rows are dynamic: JSON objects for keyed tables, JSON arrays otherwise.
    struct DataTableArgument
    {
        DataTableArgument() = default;
        explicit DataTableArgument(std::vector<nlohmann::json> rows)
            : rows(std::move(rows))
        {
        }

        std::vector<nlohmann::json> rows;
    };

    using StepArgument = std::variant<std::monostate, DocStringArgument, DataTableArgument>;

    struct ZucchiniStep
    {
        ZucchiniStep() = default;
        ZucchiniStep(std::string regex,
                     std::string methodName,
                     std::string text,
                     std::vector<Capture> captures = {},
                     StepArgument argument = {},
                     std::uint32_t line = 0,
                     std::uint32_t column = 0)
            : regex(std::move(regex))
            , methodName(std::move(methodName))
            , text(std::move(text))
            , captures(std::move(captures))
            , argument(std::move(argument))
            , line(line)
            , column(column)
        {
        }

        std::string regex;
        std::string methodName;
        std::string text;
        std::vector<Capture> captures;
        StepArgument argument;
        std::uint32_t line = 0;
        std::uint32_t column = 0;
    };

    struct Zucchini
    {
        Zucchini() = default;
        Zucchini(std::string name,
                 std::string featureName,
                 std::vector<ZucchiniStep> steps = {},
                 std::string ruleName = {},
                 std::string uri = {},
                 std::vector<std::string> tags = {})
            : name(std::move(name))
            , featureName(std::move(featureName))
            , ruleName(std::move(ruleName))
            , uri(std::move(uri))
                        , steps(std::move(steps))
              , tags(std::move(tags))
        {
        }

        std::string name;
        std::string featureName;
        std::string ruleName;
        std::string uri;
        std::vector<ZucchiniStep> steps;
        std::vector<std::string> tags;
    };

    bool operator==(const Capture& lhs, const Capture& rhs);
    bool operator==(const DocStringArgument& lhs, const DocStringArgument& rhs);
    bool operator==(const DataTableArgument& lhs, const DataTableArgument& rhs);
    bool operator==(const ZucchiniStep& lhs, const ZucchiniStep& rhs);
    bool operator==(const Zucchini& lhs, const Zucchini& rhs);

    void to_json(nlohmann::json& json, const Capture& capture);
    void from_json(const nlohmann::json& json, Capture& capture);

    void to_json(nlohmann::json& json, const DocStringArgument& docString);
    void from_json(const nlohmann::json& json, DocStringArgument& docString);

    void to_json(nlohmann::json& json, const DataTableArgument& dataTable);
    void from_json(const nlohmann::json& json, DataTableArgument& dataTable);

    void to_json(nlohmann::json& json, const ZucchiniStep& step);
    void from_json(const nlohmann::json& json, ZucchiniStep& step);

    void to_json(nlohmann::json& json, const Zucchini& zucchini);
    void from_json(const nlohmann::json& json, Zucchini& zucchini);

    std::ostream& operator<<(std::ostream& stream, const ZucchiniStep& step);
    std::ostream& operator<<(std::ostream& stream, const Zucchini& zucchini);
}
