#include "Zucchini/ManifestParser.hpp"

#include "SchemaValidator.hpp"
#include "YamlToJson.hpp"

#include <fkYAML/node.hpp>

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

namespace nZucchini
{
    namespace
    {
        using Node = fkyaml::node;

        std::string append_path(std::string path, const std::string& key)
        {
            return path.empty() ? key : path + "." + key;
        }

        std::string append_path(std::string path, std::size_t index)
        {
            const auto text = std::string("[") + std::to_string(index) + "]";
            return path.empty() ? text : path + text;
        }

        const Node* member(const Node& node, const std::string& key)
        {
            const auto& mapping = node.as_map();
            const auto found = mapping.find(Node(key));
            return found == mapping.end() ? nullptr : &found->second;
        }

        std::vector<std::string> member_names(const Node& node)
        {
            std::vector<std::string> names;
            for (const auto& entry : node.as_map())
            {
                names.push_back(entry.first.is_string() ? entry.first.get_value<std::string>() : std::string());
            }
            return names;
        }

        nlohmann::json scalar_to_json(const Node& node)
        {
            if (node.is_boolean())
            {
                return node.get_value<bool>();
            }
            if (node.is_integer())
            {
                return node.get_value<std::int64_t>();
            }
            if (node.is_float_number())
            {
                return node.get_value<double>();
            }
            if (node.is_string())
            {
                return node.get_value<std::string>();
            }
            return nlohmann::json();
        }

        class ManifestReader
        {
        public:
            explicit ManifestReader(Diagnostics& errors)
                : errors(errors)
            {
            }

            void read(const Node& root, StepDefManifest& manifest)
            {
                if (!expect_mapping(root, {}))
                {
                    return;
                }

                reject_unknown_keys(root, {}, {"includes", "types", "steps"});

                if (const auto* includes = member(root, "includes"))
                {
                    read_includes(*includes, append_path("", "includes"), manifest.includes);
                }

                if (const auto* types = member(root, "types"))
                {
                    read_types(*types, append_path("", "types"), manifest.types);
                }

                const auto* steps = member(root, "steps");
                if (steps == nullptr)
                {
                    error(append_path("", "steps"), "required key is missing");
                    return;
                }

                read_steps(*steps, append_path("", "steps"), manifest.steps);
            }

        private:
            void error(CodingPath path, std::string message)
            {
                add_diagnostic(errors, std::move(path), std::move(message));
            }

            bool expect_mapping(const Node& node, const CodingPath& path)
            {
                if (node.is_mapping())
                {
                    return true;
                }
                error(path, "expected a mapping");
                return false;
            }

            bool expect_sequence(const Node& node, const CodingPath& path)
            {
                if (node.is_sequence())
                {
                    return true;
                }
                error(path, "expected a sequence");
                return false;
            }

            void reject_unknown_keys(const Node& node,
                                     const CodingPath& path,
                                     const std::vector<std::string>& allowed)
            {
                for (const auto& name : member_names(node))
                {
                    if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
                    {
                        error(append_path(path, name), "unknown key");
                    }
                }
            }

            bool read_string(const Node& node, const CodingPath& path, std::string& value)
            {
                if (!node.is_string())
                {
                    error(path, "expected a string");
                    return false;
                }
                value = node.get_value<std::string>();
                return true;
            }

            bool read_required_string(const Node& owner,
                                      const CodingPath& path,
                                      const std::string& key,
                                      std::string& value)
            {
                const auto* node = member(owner, key);
                if (node == nullptr)
                {
                    error(append_path(path, key), "required key is missing");
                    return false;
                }
                return read_string(*node, append_path(path, key), value);
            }

            void read_optional_string(const Node& owner,
                                      const CodingPath& path,
                                      const std::string& key,
                                      std::optional<std::string>& value)
            {
                const auto* node = member(owner, key);
                if (node == nullptr || node->is_null())
                {
                    return;
                }

                std::string text;
                if (read_string(*node, append_path(path, key), text))
                {
                    value = std::move(text);
                }
            }

            void read_optional_bool(const Node& owner,
                                    const CodingPath& path,
                                    const std::string& key,
                                    bool& value)
            {
                const auto* node = member(owner, key);
                if (node == nullptr || node->is_null())
                {
                    return;
                }
                if (!node->is_boolean())
                {
                    error(append_path(path, key), "expected a boolean");
                    return;
                }
                value = node->get_value<bool>();
            }

            void read_includes(const Node& node, const CodingPath& path, std::vector<std::string>& includes)
            {
                if (!expect_sequence(node, path))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : node.as_seq())
                {
                    std::string include;
                    if (read_string(element, append_path(path, index), include))
                    {
                        includes.push_back(std::move(include));
                    }
                    ++index;
                }
            }

            void read_optional_string_sequence(const Node& owner,
                                               const CodingPath& path,
                                               const std::string& key,
                                               std::vector<std::string>& values)
            {
                const auto* node = member(owner, key);
                if (node == nullptr || node->is_null())
                {
                    return;
                }
                if (!expect_sequence(*node, append_path(path, key)))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : node->as_seq())
                {
                    std::string value;
                    if (read_string(element, append_path(append_path(path, key), index), value))
                    {
                        values.push_back(std::move(value));
                    }
                    ++index;
                }
            }

            void read_types(const Node& node, const CodingPath& path, std::vector<TypeDef>& types)
            {
                if (!expect_sequence(node, path))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : node.as_seq())
                {
                    read_type(element, append_path(path, index), types);
                    ++index;
                }
            }

            void read_type(const Node& node, const CodingPath& path, std::vector<TypeDef>& types)
            {
                if (!expect_mapping(node, path))
                {
                    return;
                }

                std::string kind;
                if (!read_required_string(node, path, "kind", kind))
                {
                    return;
                }

                if (kind == "enum")
                {
                    read_enum_type(node, path, types);
                }
                else if (kind == "struct")
                {
                    read_struct_type(node, path, types);
                }
                else
                {
                    error(append_path(path, "kind"), "expected either 'enum' or 'struct'");
                }
            }

            void read_enum_type(const Node& node, const CodingPath& path, std::vector<TypeDef>& types)
            {
                reject_unknown_keys(node, path, {"name", "kind", "prefix", "cases", "imported", "verbatimType"});

                EnumType enumType;
                if (!read_required_string(node, path, "name", enumType.name))
                {
                    return;
                }
                if (!read_required_string(node, path, "prefix", enumType.prefix))
                {
                    return;
                }
                read_optional_bool(node, path, "imported", enumType.imported);
                read_optional_string(node, path, "verbatimType", enumType.verbatimType);

                const auto* cases = member(node, "cases");
                if (cases == nullptr)
                {
                    error(append_path(path, "cases"), "required key is missing");
                    return;
                }
                if (!expect_sequence(*cases, append_path(path, "cases")))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : cases->as_seq())
                {
                    const auto casePath = append_path(append_path(path, "cases"), index);
                    ++index;

                    if (element.is_string())
                    {
                        auto name = element.get_value<std::string>();
                        enumType.cases.emplace_back(name, std::vector<std::string>{name});
                        continue;
                    }

                    if (!expect_mapping(element, casePath))
                    {
                        continue;
                    }
                    reject_unknown_keys(element, casePath, {"name", "values"});

                    EnumCase enumCase;
                    if (!read_required_string(element, casePath, "name", enumCase.name))
                    {
                        continue;
                    }

                    const auto* values = member(element, "values");
                    if (values == nullptr)
                    {
                        error(append_path(casePath, "values"), "required key is missing");
                        continue;
                    }
                    if (!expect_sequence(*values, append_path(casePath, "values")))
                    {
                        continue;
                    }

                    std::size_t valueIndex = 0;
                    for (const auto& value : values->as_seq())
                    {
                        std::string text;
                        if (read_string(value, append_path(append_path(casePath, "values"), valueIndex), text))
                        {
                            enumCase.values.push_back(std::move(text));
                        }
                        ++valueIndex;
                    }

                    enumType.cases.push_back(std::move(enumCase));
                }

                types.emplace_back(std::move(enumType));
            }

            void read_struct_type(const Node& node, const CodingPath& path, std::vector<TypeDef>& types)
            {
                reject_unknown_keys(
                    node, path, {"name", "kind", "additionalProperties", "fields", "imported", "verbatimType"});

                StructType structType;
                if (!read_required_string(node, path, "name", structType.name))
                {
                    return;
                }
                read_optional_bool(node, path, "additionalProperties", structType.additionalProperties);
                read_optional_bool(node, path, "imported", structType.imported);
                read_optional_string(node, path, "verbatimType", structType.verbatimType);

                if (const auto* fields = member(node, "fields"))
                {
                    if (!expect_sequence(*fields, append_path(path, "fields")))
                    {
                        return;
                    }

                    std::size_t index = 0;
                    for (const auto& element : fields->as_seq())
                    {
                        read_struct_field(element, append_path(append_path(path, "fields"), index), structType.fields);
                        ++index;
                    }
                }

                types.emplace_back(std::move(structType));
            }

            void read_struct_field(const Node& node, const CodingPath& path, std::vector<StructField>& fields)
            {
                if (!expect_mapping(node, path))
                {
                    return;
                }
                reject_unknown_keys(
                    node, path, {"name", "header", "optional", "default", "type", "content", "separator"});

                StructField field;
                if (!read_required_string(node, path, "name", field.name))
                {
                    return;
                }

                std::optional<std::string> type;
                read_optional_string(node, path, "type", type);
                if (type)
                {
                    field.type = *type;
                }

                read_optional_string_sequence(node, path, "header", field.headers);
                read_optional_bool(node, path, "optional", field.optional);
                read_optional_string(node, path, "content", field.content);

                if (const auto* defaultValue = member(node, "default"))
                {
                    field.defaultValue = scalar_to_json(*defaultValue);
                }

                std::optional<std::string> separator;
                read_optional_string(node, path, "separator", separator);
                if (separator)
                {
                    if (separator->size() != 1)
                    {
                        error(append_path(path, "separator"), "expected a single character");
                    }
                    else
                    {
                        field.separator = separator->front();
                    }
                }

                fields.push_back(std::move(field));
            }

            void read_steps(const Node& node, const CodingPath& path, std::vector<StepDef>& steps)
            {
                if (!expect_sequence(node, path))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : node.as_seq())
                {
                    read_step(element, append_path(path, index), steps);
                    ++index;
                }
            }

            void read_step(const Node& node, const CodingPath& path, std::vector<StepDef>& steps)
            {
                if (!expect_mapping(node, path))
                {
                    return;
                }
                reject_unknown_keys(node, path, {"step", "methodName", "arguments", "dataTable", "docstring"});

                StepDef step;
                if (!read_required_string(node, path, "step", step.step))
                {
                    return;
                }
                if (step.step.size() < 2 || step.step.front() != '^' || step.step.back() != '$')
                {
                    error(append_path(path, "step"), "step regex must be anchored with ^ and $");
                    return;
                }
                if (!read_required_string(node, path, "methodName", step.methodName))
                {
                    return;
                }

                if (const auto* arguments = member(node, "arguments"))
                {
                    read_arguments(*arguments, append_path(path, "arguments"), step.arguments);
                }
                if (const auto* dataTable = member(node, "dataTable"))
                {
                    read_data_table(*dataTable, append_path(path, "dataTable"), step.dataTable);
                }
                if (const auto* docstring = member(node, "docstring"))
                {
                    read_docstring(*docstring, append_path(path, "docstring"), step.docstring);
                }

                steps.push_back(std::move(step));
            }

            void read_arguments(const Node& node, const CodingPath& path, std::vector<Argument>& arguments)
            {
                if (!expect_sequence(node, path))
                {
                    return;
                }

                std::size_t index = 0;
                for (const auto& element : node.as_seq())
                {
                    const auto argumentPath = append_path(path, index);
                    ++index;

                    if (!expect_mapping(element, argumentPath))
                    {
                        continue;
                    }
                    reject_unknown_keys(element, argumentPath, {"name", "type"});

                    Argument argument;
                    if (!read_required_string(element, argumentPath, "name", argument.name))
                    {
                        continue;
                    }
                    if (!read_required_string(element, argumentPath, "type", argument.type))
                    {
                        continue;
                    }

                    arguments.push_back(std::move(argument));
                }
            }

            void read_data_table(const Node& node, const CodingPath& path, std::optional<DataTableSpec>& spec)
            {
                if (!expect_mapping(node, path))
                {
                    return;
                }
                reject_unknown_keys(node, path, {"direction", "header", "type"});

                DataTableSpec dataTable;
                std::optional<std::string> direction;
                read_optional_string(node, path, "direction", direction);
                if (direction)
                {
                    if (*direction == "row" || *direction == "rows")
                    {
                        dataTable.direction = TableDirection::Rows;
                    }
                    else if (*direction == "col" || *direction == "cols" || *direction == "column"
                             || *direction == "columns")
                    {
                        dataTable.direction = TableDirection::Columns;
                    }
                    else
                    {
                        error(append_path(path, "direction"), "expected one of row(s) or col(umn)(s)");
                        return;
                    }
                }

                read_optional_bool(node, path, "header", dataTable.header);
                read_optional_string(node, path, "type", dataTable.type);

                spec = std::move(dataTable);
            }

            void read_docstring(const Node& node, const CodingPath& path, std::optional<DocStringSpec>& spec)
            {
                // An empty value declares a DocString without metadata.
                if (node.is_null() || (node.is_string() && node.get_value<std::string>().empty()))
                {
                    spec = DocStringSpec(std::nullopt);
                    return;
                }

                if (node.is_boolean())
                {
                    if (node.get_value<bool>())
                    {
                        spec = DocStringSpec(std::nullopt);
                    }
                    return;
                }

                if (!expect_mapping(node, path))
                {
                    return;
                }
                reject_unknown_keys(node, path, {"contentType", "type"});

                DocStringSpec docString;
                read_optional_string(node, path, "contentType", docString.contentType);
                read_optional_string(node, path, "type", docString.type);

                if (docString.contentType && docString.contentType->empty())
                {
                    error(append_path(path, "contentType"), "must name a media type");
                    return;
                }

                if (docString.contentType && !docString.type)
                {
                    error(append_path(path, "type"), "required when 'contentType' is given");
                    return;
                }

                spec = std::move(docString);
            }

            Diagnostics& errors;
        };

        void record_parse_error(const std::string& message, Diagnostics& errors)
        {
            static const std::regex position("at line ([0-9]+), column ([0-9]+)");

            std::smatch match;
            if (std::regex_search(message, match, position))
            {
                add_diagnostic(errors,
                               {},
                               message,
                               static_cast<std::uint32_t>(std::stoul(match[1].str())),
                               static_cast<std::uint32_t>(std::stoul(match[2].str())));
                return;
            }

            add_diagnostic(errors, {}, message);
        }
    }

    bool parse_step_def_manifest(const std::string& yaml, StepDefManifest& manifest, Diagnostics& errors)
    {
        manifest = StepDefManifest();
        errors.clear();

        Node root;
        try
        {
            root = Node::deserialize(yaml);
        }
        catch (const fkyaml::exception& failure)
        {
            record_parse_error(failure.what(), errors);
            return false;
        }

        nlohmann::json document;
        std::string conversionError;
        if (!yaml_to_json(yaml, document, conversionError))
        {
            record_parse_error(conversionError, errors);
            return false;
        }

        if (!validate_against_schema(document, errors))
        {
            return false;
        }

        ManifestReader(errors).read(root, manifest);

        if (!errors.empty())
        {
            manifest = StepDefManifest();
            return false;
        }
        return true;
    }

    bool parse_step_def_manifest_from_file(const std::string& filePath,
                                           StepDefManifest& manifest,
                                           Diagnostics& errors)
    {
        manifest = StepDefManifest();
        errors.clear();

        std::ifstream file(filePath);
        if (!file)
        {
            add_diagnostic(errors, {}, "cannot open '" + filePath + "'");
            return false;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return parse_step_def_manifest(contents.str(), manifest, errors);
    }
}
