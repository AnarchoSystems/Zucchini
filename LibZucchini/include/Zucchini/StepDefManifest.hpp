#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace nZucchini {
struct EnumCase {
  EnumCase() = default;
  EnumCase(std::string name, std::vector<std::string> values = {})
      : name(std::move(name)), values(std::move(values)) {}

  std::string name;
  std::vector<std::string> values;
};

struct EnumType {
  EnumType() = default;
  EnumType(std::string name, std::string prefix, std::vector<EnumCase> cases,
           bool imported = false,
           std::optional<std::string> verbatimType = std::nullopt)
      : name(std::move(name)), prefix(std::move(prefix)),
        cases(std::move(cases)), imported(imported),
        verbatimType(std::move(verbatimType)) {}

  std::string name;
  std::string prefix;
  std::vector<EnumCase> cases;
  bool imported = false;
  std::optional<std::string> verbatimType;
};

struct StructField {
  StructField() = default;
  explicit StructField(
      std::string name, std::string type = "string",
      std::vector<std::string> headers = {}, bool optional = false,
      std::optional<nlohmann::json> defaultValue = std::nullopt,
      std::optional<std::string> content = std::nullopt, char separator = ',')
      : name(std::move(name)), type(std::move(type)),
        headers(std::move(headers)), optional(optional),
        defaultValue(std::move(defaultValue)), content(std::move(content)),
        separator(separator) {}

  std::string name;
  std::string type = "string";
  std::vector<std::string> headers;
  bool optional = false;
  std::optional<nlohmann::json> defaultValue;
  std::optional<std::string> content;
  char separator = ',';
};

struct StructType {
  StructType() = default;
  StructType(std::string name, std::vector<StructField> fields,
             bool additionalProperties = false, bool imported = false,
             std::optional<std::string> verbatimType = std::nullopt)
      : name(std::move(name)), fields(std::move(fields)),
        additionalProperties(additionalProperties), imported(imported),
        verbatimType(std::move(verbatimType)) {}

  std::string name;
  std::vector<StructField> fields;
  bool additionalProperties = false;
  bool imported = false;
  std::optional<std::string> verbatimType;
};

using TypeDef = std::variant<EnumType, StructType>;

struct Argument {
  Argument() = default;
  Argument(std::string name, std::string type)
      : name(std::move(name)), type(std::move(type)) {}

  std::string name;
  std::string type;
};

enum class TableDirection { Rows, Columns };

struct DataTableSpec {
  DataTableSpec() = default;
  explicit DataTableSpec(TableDirection direction, bool header = true,
                         std::optional<std::string> type = std::nullopt)
      : direction(direction), header(header), type(std::move(type)) {}

  TableDirection direction = TableDirection::Rows;
  bool header = true;
  // Name of a declared struct type, or "dynamic".
  std::optional<std::string> type;
};

struct DocStringSpec {
  DocStringSpec() = default;
  explicit DocStringSpec(std::optional<std::string> contentType,
                         std::optional<std::string> type = std::nullopt)
      : contentType(std::move(contentType)), type(std::move(type)) {}

  std::optional<std::string> contentType;
  std::optional<std::string> type;
};

struct StepDef {
  StepDef() = default;
  StepDef(std::string step, std::string methodName,
          std::vector<Argument> arguments = {},
          std::optional<DataTableSpec> dataTable = std::nullopt,
          std::optional<DocStringSpec> docstring = std::nullopt)
      : step(std::move(step)), methodName(std::move(methodName)),
        arguments(std::move(arguments)), dataTable(std::move(dataTable)),
        docstring(std::move(docstring)) {}

  std::string step;
  std::string methodName;
  std::vector<Argument> arguments;
  std::optional<DataTableSpec> dataTable;
  std::optional<DocStringSpec> docstring;
};

struct Definition {
  Definition() = default;
  explicit Definition(std::vector<StepDef> steps)
      : steps(std::move(steps)) {}
  Definition(std::vector<TypeDef> types, std::vector<StepDef> steps)
      : types(std::move(types)), steps(std::move(steps)) {}
  std::vector<TypeDef> types;
  std::vector<StepDef> steps;
};

const TypeDef *find_type(const Definition &manifest,
                         const std::string &name);
const std::string &type_name(const TypeDef &type);
bool operator==(const EnumCase &lhs, const EnumCase &rhs);
bool operator==(const EnumType &lhs, const EnumType &rhs);
bool operator==(const StructField &lhs, const StructField &rhs);
bool operator==(const StructType &lhs, const StructType &rhs);
bool operator==(const Argument &lhs, const Argument &rhs);
bool operator==(const DataTableSpec &lhs, const DataTableSpec &rhs);
bool operator==(const DocStringSpec &lhs, const DocStringSpec &rhs);
bool operator==(const StepDef &lhs, const StepDef &rhs);
bool operator==(const Definition &lhs, const Definition &rhs);

void to_json(nlohmann::json &json, const EnumCase &enumCase);
void to_json(nlohmann::json &json, const EnumType &enumType);
void to_json(nlohmann::json &json, const StructField &field);
void to_json(nlohmann::json &json, const StructType &structType);
void to_json(nlohmann::json &json, const TypeDef &type);
void to_json(nlohmann::json &json, const Argument &argument);
void to_json(nlohmann::json &json, const DataTableSpec &dataTable);
void to_json(nlohmann::json &json, const DocStringSpec &docString);
void to_json(nlohmann::json &json, const StepDef &step);
void to_json(nlohmann::json &json, const Definition &manifest);

std::ostream &operator<<(std::ostream &stream, const Definition &manifest);
} // namespace nZucchini
