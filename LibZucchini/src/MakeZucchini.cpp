#include "Zucchini/MakeZucchini.hpp"

#include <cucumber/messages/pickle_doc_string.hpp>
#include <cucumber/messages/pickle_step.hpp>
#include <cucumber/messages/pickle_step_argument.hpp>
#include <cucumber/messages/pickle_table.hpp>
#include <cucumber/messages/pickle_table_cell.hpp>
#include <cucumber/messages/pickle_table_row.hpp>

#include <algorithm>
#include <cctype>
#include <regex>

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
                return text == "true";
            }
            return text;
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

        Zucchini result(pickle.name, featureName);

        for (std::size_t index = 0; index < pickle.steps.size(); ++index)
        {
            const auto& pickleStep = pickle.steps[index];

            const StepDef* definition = nullptr;
            std::smatch match;
            for (std::size_t candidate = 0; candidate < manifest.steps.size(); ++candidate)
            {
                if (std::regex_match(pickleStep.text, match, patterns[candidate]))
                {
                    definition = &manifest.steps[candidate];
                    break;
                }
            }

            if (definition == nullptr)
            {
                add_diagnostic(errors,
                               step_path(index),
                               "no step definition matches '" + pickleStep.text + "'");
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
                step.captures.emplace_back(declared.name,
                                           capture_value(declared.type, match[argument + 1].str()));
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
                step.argument = DocStringArgument(docString->content, docString->media_type);
            }
            else if (dataTable != nullptr)
            {
                step.argument = DataTableArgument(rows_of(*dataTable, *definition->dataTable));
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
