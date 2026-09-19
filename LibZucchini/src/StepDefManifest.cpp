#include "Zucchini/StepDefManifest.hpp"

namespace nZucchini
{
    namespace
    {
        template <typename T>
        nlohmann::json or_null(const std::optional<T>& value)
        {
            return value ? nlohmann::json(*value) : nlohmann::json();
        }
    }
    const std::string& type_name(const TypeDef& type)
    {
        return std::visit([](const auto& value) -> const std::string& { return value.name; }, type);
    }

    const TypeDef* find_type(const StepDefManifest& manifest, const std::string& name)
    {
        for (const auto& type : manifest.types)
        {
            if (type_name(type) == name)
            {
                return &type;
            }
        }

        return nullptr;
    }

    bool operator==(const EnumCase& lhs, const EnumCase& rhs)
    {
        return lhs.name == rhs.name && lhs.values == rhs.values;
    }

    bool operator==(const EnumType& lhs, const EnumType& rhs)
    {
        return lhs.name == rhs.name && lhs.prefix == rhs.prefix && lhs.cases == rhs.cases
            && lhs.imported == rhs.imported && lhs.verbatimType == rhs.verbatimType;
    }

    bool operator==(const StructField& lhs, const StructField& rhs)
    {
        return lhs.name == rhs.name && lhs.type == rhs.type && lhs.headers == rhs.headers
            && lhs.optional == rhs.optional && lhs.defaultValue == rhs.defaultValue && lhs.content == rhs.content
            && lhs.separator == rhs.separator;
    }

    bool operator==(const StructType& lhs, const StructType& rhs)
    {
        return lhs.name == rhs.name && lhs.fields == rhs.fields
            && lhs.additionalProperties == rhs.additionalProperties && lhs.imported == rhs.imported
            && lhs.verbatimType == rhs.verbatimType;
    }

    bool operator==(const Argument& lhs, const Argument& rhs)
    {
        return lhs.name == rhs.name && lhs.type == rhs.type;
    }

    bool operator==(const DataTableSpec& lhs, const DataTableSpec& rhs)
    {
        return lhs.direction == rhs.direction && lhs.header == rhs.header && lhs.type == rhs.type;
    }

    bool operator==(const DocStringSpec& lhs, const DocStringSpec& rhs)
    {
        return lhs.contentType == rhs.contentType && lhs.type == rhs.type;
    }

    bool operator==(const StepDef& lhs, const StepDef& rhs)
    {
        return lhs.step == rhs.step && lhs.methodName == rhs.methodName && lhs.arguments == rhs.arguments
            && lhs.dataTable == rhs.dataTable && lhs.docstring == rhs.docstring;
    }

    bool operator==(const StepDefManifest& lhs, const StepDefManifest& rhs)
    {
        return lhs.includes == rhs.includes && lhs.types == rhs.types && lhs.steps == rhs.steps;
    }

    void to_json(nlohmann::json& json, const EnumCase& enumCase)
    {
        json = nlohmann::json{{"name", enumCase.name}, {"values", enumCase.values}};
    }

    void to_json(nlohmann::json& json, const EnumType& enumType)
    {
        json = nlohmann::json{{"kind", "enum"},
                              {"name", enumType.name},
                              {"prefix", enumType.prefix},
                              {"cases", enumType.cases},
                              {"imported", enumType.imported},
                              {"verbatimType", or_null(enumType.verbatimType)}};
    }

    void to_json(nlohmann::json& json, const StructField& field)
    {
        json = nlohmann::json{{"name", field.name},
                              {"type", field.type},
                              {"header", field.headers},
                              {"optional", field.optional},
                              {"default", or_null(field.defaultValue)},
                              {"content", or_null(field.content)},
                              {"separator", std::string(1, field.separator)}};
    }

    void to_json(nlohmann::json& json, const StructType& structType)
    {
        json = nlohmann::json{{"kind", "struct"},
                              {"name", structType.name},
                              {"fields", structType.fields},
                              {"additionalProperties", structType.additionalProperties},
                              {"imported", structType.imported},
                              {"verbatimType", or_null(structType.verbatimType)}};
    }

    void to_json(nlohmann::json& json, const TypeDef& type)
    {
        std::visit([&json](const auto& value) { json = value; }, type);
    }

    void to_json(nlohmann::json& json, const Argument& argument)
    {
        json = nlohmann::json{{"name", argument.name}, {"type", argument.type}};
    }

    void to_json(nlohmann::json& json, const DataTableSpec& dataTable)
    {
        json = nlohmann::json{{"direction", dataTable.direction == TableDirection::Rows ? "rows" : "columns"},
                              {"header", dataTable.header},
                              {"type", or_null(dataTable.type)}};
    }

    void to_json(nlohmann::json& json, const DocStringSpec& docString)
    {
        json = nlohmann::json{{"contentType", or_null(docString.contentType)}, {"type", or_null(docString.type)}};
    }

    void to_json(nlohmann::json& json, const StepDef& step)
    {
        json = nlohmann::json{{"step", step.step},
                              {"methodName", step.methodName},
                              {"arguments", step.arguments},
                              {"dataTable", or_null(step.dataTable)},
                              {"docstring", or_null(step.docstring)}};
    }

    void to_json(nlohmann::json& json, const StepDefManifest& manifest)
    {
        json = nlohmann::json{
            {"includes", manifest.includes}, {"types", manifest.types}, {"steps", manifest.steps}};
    }

    std::ostream& operator<<(std::ostream& stream, const StepDefManifest& manifest)
    {
        return stream << nlohmann::json(manifest).dump(2);
    }
}
