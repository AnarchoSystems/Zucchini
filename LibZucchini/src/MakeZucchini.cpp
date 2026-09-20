#include "Zucchini/MakeZucchini.hpp"

#include "Zucchini/MediaType.hpp"

#include <cucumber/messages/pickle_doc_string.hpp>
#include <cucumber/messages/pickle_step.hpp>
#include <cucumber/messages/pickle_step_argument.hpp>
#include <cucumber/messages/pickle_table.hpp>
#include <cucumber/messages/pickle_table_cell.hpp>
#include <cucumber/messages/pickle_table_row.hpp>

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <sstream>

namespace nZucchini
{
    namespace
    {
        namespace messages = cucumber::messages;

        std::string append_path(std::string path, const std::string& key)
        {
            return path.empty() ? key : path + "." + key;
        }

        std::string append_path(std::string path, std::size_t index)
        {
            const auto indexText = std::string("[") + std::to_string(index) + "]";
            return path.empty() ? indexText : path + indexText;
        }

        std::string step_path(std::size_t index)
        {
            return append_path("steps", index);
        }

        std::string step_path(std::size_t index, std::string key)
        {
            return append_path(step_path(index), key);
        }

        const EnumType* find_enum(const StepDefManifest& manifest, const std::string& name)
        {
            const auto* type = find_type(manifest, name);
            return type == nullptr ? nullptr : std::get_if<EnumType>(type);
        }

        const StructType* find_struct(const StepDefManifest& manifest, const std::string& name)
        {
            const auto* type = find_type(manifest, name);
            return type == nullptr ? nullptr : std::get_if<StructType>(type);
        }

        std::string expected_headers(const StructField& field)
        {
            const auto headers = field.headers.empty() ? std::vector<std::string>{field.name} : field.headers;
            std::string result;
            for (const auto& header : headers)
            {
                result += (result.empty() ? "'" : ", '") + header + "'";
            }
            return result;
        }

        bool validate_text_value(const StepDefManifest& manifest,
                                 const std::string& type,
                                 const std::string& text,
                                 std::string& error)
        {
            try
            {
                std::size_t consumed = 0;
                if (type == "int" || type == "integer" || type == "long")
                {
                    std::stoll(text, &consumed);
                    if (consumed != text.size())
                    {
                        throw std::invalid_argument("trailing characters");
                    }
                    return true;
                }
                if (type == "float" || type == "double")
                {
                    std::stod(text, &consumed);
                    if (consumed != text.size())
                    {
                        throw std::invalid_argument("trailing characters");
                    }
                    return true;
                }
            }
            catch (const std::exception&)
            {
                error = "cannot parse '" + text + "' as " + type;
                return false;
            }

            if (type == "bool" || type == "boolean")
            {
                if (text == "true" || text == "false" || text == "1" || text == "0" || text == "yes"
                    || text == "no")
                {
                    return true;
                }
                error = "cannot parse '" + text + "' as " + type;
                return false;
            }
            if (type.empty() || type == "string" || type == "ignore")
            {
                return true;
            }
            if (const auto* enumeration = find_enum(manifest, type))
            {
                for (const auto& enumCase : enumeration->cases)
                {
                    if (std::find(enumCase.values.begin(), enumCase.values.end(), text) != enumCase.values.end())
                    {
                        return true;
                    }
                }
                error = "cannot parse '" + text + "' as " + type;
                return false;
            }
            return true;
        }

        nlohmann::json capture_value(const std::string& type, const std::string& text)
        {
            if (type == "int" || type == "integer" || type == "long")
            {
                return static_cast<std::int64_t>(std::stoll(text));
            }
            if (type == "float" || type == "double")
            {
                return std::stod(text);
            }
            if (type == "bool" || type == "boolean")
            {
                return text == "true" || text == "1" || text == "yes";
            }
            return text;
        }

        std::vector<std::string> split(const std::string& value, char separator)
        {
            std::vector<std::string> parts;
            std::string part;
            std::istringstream stream(value);
            while (std::getline(stream, part, separator))
            {
                parts.push_back(part);
            }
            return parts;
        }

        void validate_table(const StepDefManifest& manifest,
                            const DataTableSpec& spec,
                            const std::vector<nlohmann::json>& rows,
                            std::size_t stepIndex,
                            Diagnostics& errors)
        {
            if (!spec.type || *spec.type == "dynamic")
            {
                return;
            }

            const auto* structure = find_struct(manifest, *spec.type);
            if (structure == nullptr)
            {
                add_diagnostic(errors,
                               step_path(stepIndex, "dataTable"),
                               "table type '" + *spec.type + "' is not a declared struct");
                return;
            }

            std::set<std::string> knownHeaders;
            for (const auto& field : structure->fields)
            {
                const auto headers = field.headers.empty() ? std::vector<std::string>{field.name} : field.headers;
                knownHeaders.insert(headers.begin(), headers.end());
            }

            for (std::size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
            {
                const auto& row = rows[rowIndex];
                const auto rowPath = append_path(step_path(stepIndex, "dataTable"), rowIndex);
                for (std::size_t fieldIndex = 0; fieldIndex < structure->fields.size(); ++fieldIndex)
                {
                    const auto& field = structure->fields[fieldIndex];
                    if (field.type == "ignore")
                    {
                        continue;
                    }

                    std::optional<std::string> value;
                    if (row.is_object())
                    {
                        const auto headers = field.headers.empty() ? std::vector<std::string>{field.name} : field.headers;
                        for (const auto& header : headers)
                        {
                            const auto cell = row.find(header);
                            if (cell != row.end())
                            {
                                value = cell->get<std::string>();
                                break;
                            }
                        }
                    }
                    else if (fieldIndex < row.size())
                    {
                        value = row[fieldIndex].get<std::string>();
                    }

                    if (!value)
                    {
                        if (!field.optional && !field.defaultValue)
                        {
                            add_diagnostic(errors,
                                           append_path(rowPath, field.name),
                                           "required table column is missing; expected one of: "
                                               + expected_headers(field));
                        }
                        continue;
                    }

                    if (field.optional && value->empty())
                    {
                        continue;
                    }

                    std::string error;
                    if (field.type == "list")
                    {
                        const auto content = field.content.value_or("string");
                        for (const auto& item : split(*value, field.separator))
                        {
                            if (!validate_text_value(manifest, content, item, error))
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        validate_text_value(manifest, field.type, *value, error);
                    }
                    if (!error.empty())
                    {
                        add_diagnostic(errors, append_path(rowPath, field.name), std::move(error));
                    }
                }

                if (row.is_object() && !structure->additionalProperties)
                {
                    for (const auto& cell : row.items())
                    {
                        if (knownHeaders.find(cell.key()) == knownHeaders.end())
                        {
                            add_diagnostic(errors,
                                           append_path(rowPath, cell.key()),
                                           "unknown table column '" + cell.key() + "'");
                        }
                    }
                }
            }
        }

        void validate_json_value(const StepDefManifest& manifest,
                                 const std::string& type,
                                 const nlohmann::json& value,
                                 const CodingPath& path,
                                 Diagnostics& errors);

        void validate_json_struct(const StepDefManifest& manifest,
                                  const StructType& structure,
                                  const nlohmann::json& value,
                                  const CodingPath& path,
                                  Diagnostics& errors)
        {
            if (!value.is_object())
            {
                add_diagnostic(errors, path, "expected an object for type '" + structure.name + "'");
                return;
            }

            std::set<std::string> knownFields;
            for (const auto& field : structure.fields)
            {
                knownFields.insert(field.name);
                const auto member = value.find(field.name);
                if (member == value.end() || member->is_null())
                {
                    if (!field.optional && !field.defaultValue)
                    {
                        add_diagnostic(errors, append_path(path, field.name), "required property is missing");
                    }
                    continue;
                }
                const auto fieldPath = append_path(path, field.name);
                if (field.type == "list")
                {
                    if (!member->is_array())
                    {
                        add_diagnostic(errors, fieldPath, "expected an array");
                        continue;
                    }
                    const auto content = field.content.value_or("string");
                    for (std::size_t index = 0; index < member->size(); ++index)
                    {
                        validate_json_value(manifest,
                                            content,
                                            (*member)[index],
                                            append_path(fieldPath, index),
                                            errors);
                    }
                    continue;
                }
                validate_json_value(manifest, field.type, *member, fieldPath, errors);
            }

            if (!structure.additionalProperties)
            {
                for (const auto& member : value.items())
                {
                    if (knownFields.find(member.key()) == knownFields.end())
                    {
                        add_diagnostic(errors,
                                       append_path(path, member.key()),
                                       "unknown property '" + member.key() + "'");
                    }
                }
            }
        }

        void validate_json_value(const StepDefManifest& manifest,
                                 const std::string& type,
                                 const nlohmann::json& value,
                                 const CodingPath& path,
                                 Diagnostics& errors)
        {
            if (type.empty() || type == "string")
            {
                if (!value.is_string()) add_diagnostic(errors, path, "expected a string");
                return;
            }
            if (type == "int" || type == "integer" || type == "long")
            {
                if (!value.is_number_integer()) add_diagnostic(errors, path, "expected an integer");
                return;
            }
            if (type == "float" || type == "double")
            {
                if (!value.is_number()) add_diagnostic(errors, path, "expected a number");
                return;
            }
            if (type == "bool" || type == "boolean")
            {
                if (!value.is_boolean()) add_diagnostic(errors, path, "expected a boolean");
                return;
            }
            if (type == "list")
            {
                if (!value.is_array()) add_diagnostic(errors, path, "expected an array");
                return;
            }
            if (const auto* enumeration = find_enum(manifest, type))
            {
                if (!value.is_string())
                {
                    add_diagnostic(errors, path, "expected a string for enum '" + type + "'");
                    return;
                }
                std::string error;
                if (!validate_text_value(manifest, type, value.get<std::string>(), error))
                {
                    add_diagnostic(errors, path, std::move(error));
                }
                return;
            }
            if (const auto* structure = find_struct(manifest, type))
            {
                validate_json_struct(manifest, *structure, value, path, errors);
            }
        }

        void validate_doc_string(const StepDefManifest& manifest,
                                 const DocStringSpec& spec,
                                 const messages::pickle_doc_string& docString,
                                 std::size_t stepIndex,
                                 Diagnostics& errors)
        {
            if (!spec.type)
            {
                return;
            }

            const auto contentType = spec.contentType.value_or("json");
            if (contentType != "json" && contentType != "yaml")
            {
                return;
            }

            const auto path = step_path(stepIndex, "docstring");
            try
            {
                const auto document = contentType == "yaml"
                                          ? MediaTypeConverter<yaml>::convertToJSON(docString.content)
                                          : MediaTypeConverter<json>::convertToJSON(docString.content);
                validate_json_value(manifest, *spec.type, document, path, errors);
            }
            catch (const std::exception& failure)
            {
                add_diagnostic(errors, path, failure.what());
            }
        }

        // A DocString may declare no media type at all; if it declares one it has to match.
        bool doc_string_matches(const DocStringSpec& spec,
                                const messages::pickle_doc_string& docString,
                                std::string& error)
        {
            if (!spec.contentType || !docString.media_type || docString.media_type->empty())
            {
                return true;
            }

            std::string normalized;
            for (const auto character : *docString.media_type)
            {
                normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
            }

            const auto suffix = normalized.rfind('/');
            if (suffix != std::string::npos)
            {
                normalized = normalized.substr(suffix + 1);
            }
            if (normalized == "yml")
            {
                normalized = "yaml";
            }

            if (normalized == *spec.contentType)
            {
                return true;
            }

            error = "DocString media type '" + *docString.media_type + "' does not match the declared '"
                + *spec.contentType + "'";
            return false;
        }

        std::vector<std::vector<std::string>> cells_of(const messages::pickle_table& table)
        {
            std::vector<std::vector<std::string>> cells;
            for (const auto& row : table.rows)
            {
                std::vector<std::string> values;
                for (const auto& cell : row.cells)
                {
                    values.push_back(cell.value);
                }
                cells.push_back(std::move(values));
            }
            return cells;
        }

        std::vector<nlohmann::json> rows_of(const messages::pickle_table& table, const DataTableSpec& spec)
        {
            const auto cells = cells_of(table);
            std::vector<nlohmann::json> rows;

            if (spec.direction == TableDirection::Rows)
            {
                const auto first = spec.header ? std::size_t{1} : std::size_t{0};
                for (std::size_t index = first; index < cells.size(); ++index)
                {
                    nlohmann::json row = spec.header ? nlohmann::json::object() : nlohmann::json::array();
                    for (std::size_t column = 0; column < cells[index].size(); ++column)
                    {
                        if (spec.header)
                        {
                            if (column < cells.front().size())
                            {
                                row[cells.front()[column]] = cells[index][column];
                            }
                        }
                        else
                        {
                            row.push_back(cells[index][column]);
                        }
                    }
                    rows.push_back(std::move(row));
                }
                return rows;
            }

            std::size_t width = 0;
            for (const auto& row : cells)
            {
                width = std::max(width, row.size());
            }

            const auto first = spec.header ? std::size_t{1} : std::size_t{0};
            for (std::size_t column = first; column < width; ++column)
            {
                nlohmann::json record = spec.header ? nlohmann::json::object() : nlohmann::json::array();
                for (const auto& row : cells)
                {
                    if (column >= row.size())
                    {
                        continue;
                    }
                    if (spec.header)
                    {
                        record[row.front()] = row[column];
                    }
                    else
                    {
                        record.push_back(row[column]);
                    }
                }
                rows.push_back(std::move(record));
            }
            return rows;
        }
    }

    const StepDef* find_step_def(const StepDefManifest& manifest, const std::string& text)
    {
        for (const auto& definition : manifest.steps)
        {
            try
            {
                if (std::regex_match(text, std::regex(definition.step, std::regex::ECMAScript)))
                {
                    return &definition;
                }
            }
            catch (const std::regex_error&)
            {
                continue;
            }
        }
        return nullptr;
    }

    bool make_zucchini(const messages::pickle& pickle,
                       const StepDefManifest& manifest,
                       const std::string& featureName,
                       Zucchini& zucchini,
                       Diagnostics& errors)
    {
        zucchini = Zucchini();
        errors.clear();

        std::vector<std::regex> patterns;
        patterns.reserve(manifest.steps.size());
        for (std::size_t index = 0; index < manifest.steps.size(); ++index)
        {
            try
            {
                patterns.emplace_back(manifest.steps[index].step, std::regex::ECMAScript);
            }
            catch (const std::regex_error& failure)
            {
                add_diagnostic(errors,
                               append_path(append_path("steps", index), "step"),
                               std::string("invalid step regex: ") + failure.what());
                return false;
            }
        }

        std::vector<std::string> tags;
        tags.reserve(pickle.tags.size());
        for (const auto& tag : pickle.tags)
        {
            tags.push_back(tag.name);
        }

        Zucchini result(pickle.name, featureName, {}, {}, {}, std::move(tags));

        for (std::size_t index = 0; index < pickle.steps.size(); ++index)
        {
            const auto& pickleStep = pickle.steps[index];

            const StepDef* definition = nullptr;
            std::smatch match;
            std::size_t matchCount = 0;
            for (std::size_t candidate = 0; candidate < manifest.steps.size(); ++candidate)
            {
                std::smatch candidateMatch;
                if (std::regex_match(pickleStep.text, candidateMatch, patterns[candidate]))
                {
                    definition = &manifest.steps[candidate];
                    match = std::move(candidateMatch);
                    ++matchCount;
                }
            }

            if (definition == nullptr)
            {
                add_diagnostic(errors,
                               step_path(index),
                               "no step definition matches '" + pickleStep.text + "'");
                continue;
            }

            if (matchCount > 1)
            {
                add_diagnostic(errors,
                               step_path(index),
                               "multiple step definitions match '" + pickleStep.text + "'");
                continue;
            }

            ZucchiniStep step(definition->step, definition->methodName, pickleStep.text);

            if (definition->arguments.size() + 1 != match.size())
            {
                add_diagnostic(errors,
                               step_path(index, "arguments"),
                               "the step regex has " + std::to_string(match.size() - 1)
                                   + " capture group(s) but " + std::to_string(definition->arguments.size())
                                   + " argument(s) are declared");
                continue;
            }

            for (std::size_t argument = 0; argument < definition->arguments.size(); ++argument)
            {
                const auto& declared = definition->arguments[argument];
                if (declared.type == "ignore")
                {
                    continue;
                }
                const auto text = match[argument + 1].str();
                std::string error;
                if (!validate_text_value(manifest, declared.type, text, error))
                {
                    add_diagnostic(errors,
                                   append_path(step_path(index, "arguments"), argument),
                                   std::move(error));
                    continue;
                }
                step.captures.emplace_back(declared.name, capture_value(declared.type, text));
            }

            const auto* docString = pickleStep.argument && pickleStep.argument->doc_string
                                        ? &*pickleStep.argument->doc_string
                                        : nullptr;
            const auto* dataTable = pickleStep.argument && pickleStep.argument->data_table
                                        ? &*pickleStep.argument->data_table
                                        : nullptr;

            if (docString != nullptr && dataTable != nullptr)
            {
                add_diagnostic(errors, step_path(index), "a step cannot take both a DocString and a data table");
                continue;
            }

            if ((docString != nullptr) != definition->docstring.has_value())
            {
                add_diagnostic(errors,
                               step_path(index, "docstring"),
                               docString != nullptr ? "the step definition declares no DocString"
                                                    : "the step definition requires a DocString");
                continue;
            }

            if ((dataTable != nullptr) != definition->dataTable.has_value())
            {
                add_diagnostic(errors,
                               step_path(index, "dataTable"),
                               dataTable != nullptr ? "the step definition declares no data table"
                                                    : "the step definition requires a data table");
                continue;
            }

            if (docString != nullptr)
            {
                std::string error;
                if (!doc_string_matches(*definition->docstring, *docString, error))
                {
                    add_diagnostic(errors, step_path(index, "docstring"), error);
                    continue;
                }
                validate_doc_string(manifest, *definition->docstring, *docString, index, errors);
                step.argument = DocStringArgument(docString->content, docString->media_type);
            }
            else if (dataTable != nullptr)
            {
                auto rows = rows_of(*dataTable, *definition->dataTable);
                validate_table(manifest, *definition->dataTable, rows, index, errors);
                step.argument = DataTableArgument(std::move(rows));
            }

            result.steps.push_back(std::move(step));
        }

        if (!errors.empty())
        {
            return false;
        }

        zucchini = std::move(result);
        return true;
    }
}
