#include "Zucchini/StepDefinitions.hpp"

namespace nZucchini {
namespace {
template <typename T> nlohmann::json or_null(const std::optional<T> &value) {
  return value ? nlohmann::json(*value) : nlohmann::json();
}

template <typename T>
void read_optional(const nlohmann::json &json, const char *key,
                   std::optional<T> &value) {
  const auto found = json.find(key);
  if (found == json.end() || found->is_null()) {
    value.reset();
  } else {
    value = found->template get<T>();
  }
}
} // namespace

const std::string &type_name(const TypeDef &type) {
  return std::visit(
      [](const auto &value) -> const std::string & { return value.name; },
      type);
}

const TypeDef *find_type(const StepDefinitions &definitions,
                         const std::string &name) {
  for (const auto &type : definitions.types) {
    if (type_name(type) == name) {
      return &type;
    }
  }
  return nullptr;
}

bool operator==(const EnumCase &lhs, const EnumCase &rhs) {
  return lhs.name == rhs.name && lhs.values == rhs.values;
}

bool operator==(const EnumType &lhs, const EnumType &rhs) {
  return lhs.name == rhs.name && lhs.prefix == rhs.prefix &&
         lhs.cases == rhs.cases && lhs.imported == rhs.imported &&
         lhs.verbatimType == rhs.verbatimType;
}

bool operator==(const StructField &lhs, const StructField &rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type &&
         lhs.headers == rhs.headers && lhs.optional == rhs.optional &&
         lhs.defaultValue == rhs.defaultValue && lhs.content == rhs.content &&
         lhs.separator == rhs.separator;
}

bool operator==(const StructType &lhs, const StructType &rhs) {
  return lhs.name == rhs.name && lhs.fields == rhs.fields &&
         lhs.additionalProperties == rhs.additionalProperties &&
         lhs.imported == rhs.imported && lhs.verbatimType == rhs.verbatimType;
}

bool operator==(const Argument &lhs, const Argument &rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type;
}

bool operator==(const DataTableSpec &lhs, const DataTableSpec &rhs) {
  return lhs.direction == rhs.direction && lhs.header == rhs.header &&
         lhs.type == rhs.type;
}

bool operator==(const DocStringSpec &lhs, const DocStringSpec &rhs) {
  return lhs.contentType == rhs.contentType && lhs.type == rhs.type;
}

bool operator==(const StepDefinition &lhs, const StepDefinition &rhs) {
  return lhs.step == rhs.step && lhs.methodName == rhs.methodName &&
         lhs.arguments == rhs.arguments && lhs.dataTable == rhs.dataTable &&
         lhs.docstring == rhs.docstring;
}

bool operator==(const StepDefinitions &lhs, const StepDefinitions &rhs) {
  return lhs.types == rhs.types && lhs.steps == rhs.steps;
}

void to_json(nlohmann::json &json, const EnumCase &enumCase) {
  json = nlohmann::json{{"name", enumCase.name}, {"values", enumCase.values}};
}

void to_json(nlohmann::json &json, const EnumType &enumType) {
  json = nlohmann::json{{"kind", "enum"},
                        {"name", enumType.name},
                        {"prefix", enumType.prefix},
                        {"cases", enumType.cases},
                        {"imported", enumType.imported},
                        {"verbatimType", or_null(enumType.verbatimType)}};
}

void to_json(nlohmann::json &json, const StructField &field) {
  json = nlohmann::json{{"name", field.name},
                        {"type", field.type},
                        {"header", field.headers},
                        {"optional", field.optional},
                        {"default", or_null(field.defaultValue)},
                        {"content", or_null(field.content)},
                        {"separator", std::string(1, field.separator)}};
}

void to_json(nlohmann::json &json, const StructType &structType) {
  json = nlohmann::json{{"kind", "struct"},
                        {"name", structType.name},
                        {"fields", structType.fields},
                        {"additionalProperties",
                         structType.additionalProperties},
                        {"imported", structType.imported},
                        {"verbatimType", or_null(structType.verbatimType)}};
}

void to_json(nlohmann::json &json, const TypeDef &type) {
  std::visit([&json](const auto &value) { json = value; }, type);
}

void to_json(nlohmann::json &json, const Argument &argument) {
  json = nlohmann::json{{"name", argument.name}, {"type", argument.type}};
}

void to_json(nlohmann::json &json, const DataTableSpec &dataTable) {
  json = nlohmann::json{
      {"direction",
       dataTable.direction == TableDirection::Rows ? "rows" : "columns"},
      {"header", dataTable.header},
      {"type", or_null(dataTable.type)}};
}

void to_json(nlohmann::json &json, const DocStringSpec &docString) {
  json = nlohmann::json{{"contentType", or_null(docString.contentType)},
                        {"type", or_null(docString.type)}};
}

void to_json(nlohmann::json &json, const StepDefinition &step) {
  json = nlohmann::json{{"step", step.step},
                        {"methodName", step.methodName},
                        {"arguments", step.arguments},
                        {"dataTable", or_null(step.dataTable)},
                        {"docstring", or_null(step.docstring)}};
}

void to_json(nlohmann::json &json, const StepDefinitions &definitions) {
  json = nlohmann::json{{"types", definitions.types},
                        {"steps", definitions.steps}};
}

void from_json(const nlohmann::json &json, EnumCase &enumCase) {
  json.at("name").get_to(enumCase.name);
  json.at("values").get_to(enumCase.values);
}

void from_json(const nlohmann::json &json, EnumType &enumType) {
  json.at("name").get_to(enumType.name);
  json.at("prefix").get_to(enumType.prefix);
  json.at("cases").get_to(enumType.cases);
  json.at("imported").get_to(enumType.imported);
  read_optional(json, "verbatimType", enumType.verbatimType);
}

void from_json(const nlohmann::json &json, StructField &field) {
  json.at("name").get_to(field.name);
  json.at("type").get_to(field.type);
  json.at("header").get_to(field.headers);
  json.at("optional").get_to(field.optional);
  read_optional(json, "default", field.defaultValue);
  read_optional(json, "content", field.content);
  const auto separator = json.at("separator").get<std::string>();
  if (separator.size() != 1) {
    throw nlohmann::json::type_error::create(
        302, "separator must contain exactly one character", &json);
  }
  field.separator = separator.front();
}

void from_json(const nlohmann::json &json, StructType &structType) {
  json.at("name").get_to(structType.name);
  json.at("fields").get_to(structType.fields);
  json.at("additionalProperties").get_to(structType.additionalProperties);
  json.at("imported").get_to(structType.imported);
  read_optional(json, "verbatimType", structType.verbatimType);
}

void from_json(const nlohmann::json &json, TypeDef &type) {
  const auto &kind = json.at("kind").get_ref<const std::string &>();
  if (kind == "enum") {
    type = json.get<EnumType>();
  } else if (kind == "struct") {
    type = json.get<StructType>();
  } else {
    throw nlohmann::json::type_error::create(
        302, "type kind must be either 'enum' or 'struct'", &json);
  }
}

void from_json(const nlohmann::json &json, Argument &argument) {
  json.at("name").get_to(argument.name);
  json.at("type").get_to(argument.type);
}

void from_json(const nlohmann::json &json, DataTableSpec &dataTable) {
  const auto &direction =
      json.at("direction").get_ref<const std::string &>();
  if (direction == "rows") {
    dataTable.direction = TableDirection::Rows;
  } else if (direction == "columns") {
    dataTable.direction = TableDirection::Columns;
  } else {
    throw nlohmann::json::type_error::create(
        302, "table direction must be either 'rows' or 'columns'", &json);
  }
  json.at("header").get_to(dataTable.header);
  read_optional(json, "type", dataTable.type);
}

void from_json(const nlohmann::json &json, DocStringSpec &docString) {
  read_optional(json, "contentType", docString.contentType);
  read_optional(json, "type", docString.type);
}

void from_json(const nlohmann::json &json, StepDefinition &step) {
  json.at("step").get_to(step.step);
  json.at("methodName").get_to(step.methodName);
  json.at("arguments").get_to(step.arguments);
  read_optional(json, "dataTable", step.dataTable);
  read_optional(json, "docstring", step.docstring);
}

void from_json(const nlohmann::json &json, StepDefinitions &definitions) {
  json.at("types").get_to(definitions.types);
  json.at("steps").get_to(definitions.steps);
}

std::ostream &operator<<(std::ostream &stream,
                         const StepDefinitions &definitions) {
  return stream << nlohmann::json(definitions).dump(2);
}
} // namespace nZucchini