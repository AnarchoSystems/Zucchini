#include "Lowering.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace nZucchini {
namespace {
namespace model = nZucchiniTemplates;

struct CppType {
  std::string declType;  // how the value is declared
  std::string paramType; // how it is passed to a step method
  std::string decoder;   // "%" is replaced by the string expression to decode
};

std::string substitute(const std::string &pattern, const std::string &value) {
  const auto placeholder = pattern.find('%');
  if (placeholder == std::string::npos) {
    return pattern;
  }
  return pattern.substr(0, placeholder) + value +
         pattern.substr(placeholder + 1);
}

std::string escape(const std::string &text) {
  std::string escaped;
  for (const auto character : text) {
    if (character == '\\' || character == '"') {
      escaped.push_back('\\');
    }
    escaped.push_back(character);
  }
  return escaped;
}

std::string quote(const std::string &text) { return '"' + escape(text) + '"'; }

const EnumType *find_enum(const StepDefinitions &manifest,
                          const std::string &name) {
  const auto *type = find_type(manifest, name);
  return type == nullptr ? nullptr : std::get_if<EnumType>(type);
}

const StructType *find_struct(const StepDefinitions &manifest,
                              const std::string &name) {
  const auto *type = find_type(manifest, name);
  return type == nullptr ? nullptr : std::get_if<StructType>(type);
}

std::string enum_cpp_name(const Stylesheet &stylesheet,
                          const EnumType &enumeration) {
  if (enumeration.verbatimType) {
    return *enumeration.verbatimType;
  }
  if (enumeration.imported) {
    return enumeration.name;
  }
  return compose_type_name(stylesheet.typeNaming, enumeration.name,
                           /*isEnum=*/true);
}

std::string struct_cpp_name(const Stylesheet &stylesheet,
                            const StructType &structure) {
  if (structure.verbatimType) {
    return *structure.verbatimType;
  }
  if (structure.imported) {
    return structure.name;
  }
  return compose_type_name(stylesheet.typeNaming, structure.name,
                           /*isEnum=*/false);
}

std::string cpp_symbol_name(const std::string &type) {
  const auto pos = type.rfind("::");
  return pos == std::string::npos ? type : type.substr(pos + 2);
}

std::string string_class(const Stylesheet &stylesheet) {
  return stylesheet.stringClass.value_or("std::string");
}

// The "kind" condition (onStruct/onEnum/onInt/onDouble/onBool/onString) of a
// manifest type name; a list's kind is the kind of its element type, since
// list-ness has its own separate condition.
VariableKind
kind_of_type(const StepDefinitions &manifest, const std::string &type,
             const std::optional<std::string> &content = std::nullopt) {
  if (type == "int" || type == "integer" || type == "long") {
    return VariableKind::Int;
  }
  if (type == "float" || type == "double") {
    return VariableKind::Double;
  }
  if (type == "bool" || type == "boolean") {
    return VariableKind::Bool;
  }
  if (type == "list") {
    return kind_of_type(manifest, content.value_or("string"));
  }
  if (find_enum(manifest, type) != nullptr) {
    return VariableKind::Enum;
  }
  if (find_struct(manifest, type) != nullptr) {
    return VariableKind::Struct;
  }
  return VariableKind::String;
}

CppType cpp_type(const StepDefinitions &manifest, const Stylesheet &stylesheet,
                 const std::string &type) {
  if (type == "int" || type == "integer" || type == "long") {
    return {"long", "long", "to_long(%)"};
  }
  if (type == "float" || type == "double") {
    return {"double", "double", "to_double(%)"};
  }
  if (type == "bool" || type == "boolean") {
    return {"bool", "bool", "to_bool(%)"};
  }
  if (type.empty() || type == "string") {
    const auto stringClass = string_class(stylesheet);
    // Custom string classes may only accept a C string, so hand them
    // (%).c_str() rather than %.
    const auto decoder =
        stringClass == "std::string" ? "%" : stringClass + "((%).c_str())";
    return {stringClass, "const " + stringClass + "&", decoder};
  }
  if (const auto *enumeration = find_enum(manifest, type)) {
    const auto name = enum_cpp_name(stylesheet, *enumeration);
    return {name, name, "parse_" + name + "(%)"};
  }
  throw std::runtime_error("unknown type '" + type + "'");
}

model::FieldDef lower_field(const StepDefinitions &manifest,
                            const Stylesheet &stylesheet,
                            const StructField &field) {
  model::FieldDef lowered;
  lowered.name = field.name;
  const auto isList = field.type == "list";
  const auto kind = kind_of_type(
      manifest, isList ? field.content.value_or("string") : field.type);
  lowered.cppName =
      compose_variable_name(stylesheet.variableNaming, field.name, kind,
                            /*isMember=*/true, isList, field.optional);
  lowered.headers = field.headers.empty() ? std::vector<std::string>{field.name}
                                          : field.headers;
  lowered.header = lowered.headers.front();
  lowered.isOptional = field.optional;
  lowered.hasDefault = false;

  const auto headers = [&]() {
    std::string value = "std::vector<std::string>{";
    for (std::size_t i = 0; i < lowered.headers.size(); ++i) {
      if (i != 0)
        value += ", ";
      value += quote(lowered.headers[i]);
    }
    return value + "}";
  }();

  if (field.type == "list") {
    const auto separator = std::string("'") + field.separator + "'";
    const auto split =
        "split_cell(require_cell(row, " + headers + "), " + separator + ")";
    const auto content = field.content.value_or("string");

    if (content.empty() || content == "string") {
      const auto stringClass = string_class(stylesheet);
      lowered.declType = "std::vector<" + stringClass + ">";
      lowered.valueType = lowered.declType;
      if (stringClass == "std::string") {
        lowered.reader = split;
      } else {
        // Build elements via .c_str() so string classes that only take a const
        // char* work too.
        lowered.reader =
            "[&]{ std::vector<" + stringClass +
            "> elements; for (const auto& part : " + split +
            ") elements.emplace_back(part.c_str()); return elements; }()";
      }
      return lowered;
    }

    const auto *enumeration = find_enum(manifest, content);
    if (enumeration == nullptr) {
      throw std::runtime_error("list content type '" + content +
                               "' is not a declared enum");
    }

    const auto element = enum_cpp_name(stylesheet, *enumeration);
    lowered.declType = "std::vector<" + element + ">";
    lowered.valueType = lowered.declType;
    lowered.reader =
        "parse_list_" + cpp_symbol_name(element) + "(" + split + ")";
    return lowered;
  }

  const auto type = cpp_type(manifest, stylesheet, field.type);
  lowered.valueType = type.declType;
  lowered.declType =
      field.optional ? "std::optional<" + type.declType + ">" : type.declType;

  if (field.optional) {
    // A missing column and an empty cell both mean "no value".
    lowered.reader =
        "cell_or(row, " + headers +
        ", \"\").empty() ? std::nullopt : std::optional<" + type.declType +
        ">(" + substitute(type.decoder, "require_cell(row, " + headers + ")") +
        ")";
  } else if (field.defaultValue) {
    const auto fallback = field.defaultValue->is_string()
                              ? field.defaultValue->get<std::string>()
                              : field.defaultValue->dump();
    lowered.hasDefault = true;
    // The decoder may call .c_str() on its argument, so give it a std::string,
    // not a bare literal.
    lowered.defaultCode =
        substitute(type.decoder, "std::string(" + quote(fallback) + ")");
    lowered.reader = substitute(type.decoder, "cell_or(row, " + headers + ", " +
                                                  quote(fallback) + ")");
  } else {
    lowered.reader =
        substitute(type.decoder, "require_cell(row, " + headers + ")");
  }

  return lowered;
}

model::Argument lower_capture(const StepDefinitions &manifest,
                              const Stylesheet &stylesheet,
                              const Argument &argument, std::size_t index,
                              std::string &parameters) {
  const auto type = cpp_type(manifest, stylesheet, argument.type);
  const auto name = compose_variable_name(
      stylesheet.variableNaming, argument.name,
      kind_of_type(manifest, argument.type),
      /*isMember=*/false, /*isList=*/false, /*isOptional=*/false);
  const auto source = "step.captures.at(" + std::to_string(index) +
                      ").value.get<std::string>()";
  const auto numeric = "step.captures.at(" + std::to_string(index) + ").value";

  std::string decoded;
  if (type.declType == "long" || type.declType == "double" ||
      type.declType == "bool") {
    decoded = numeric + ".get<" + type.declType + ">()";
  } else {
    decoded = substitute(type.decoder, source);
  }

  if (!parameters.empty()) {
    parameters += ", ";
  }
  parameters += type.paramType + " " + name;

  model::Argument lowered;
  lowered.name = name;
  lowered.declaration =
      "const " + type.declType + " " + name + " = " + decoded + ";";
  return lowered;
}

void lower_data_table(const StepDefinitions &manifest,
                      const Stylesheet &stylesheet, const DataTableSpec &spec,
                      std::string &parameters,
                      std::vector<model::Argument> &arguments) {
  std::string rowType = "Row";
  std::string decoder = "dynamic_rows(step)";

  if (!spec.header) {
    rowType = "std::vector<std::string>";
    decoder = "positional_rows(step)";
  }

  if (spec.type && *spec.type != "dynamic") {
    const auto *structure = find_struct(manifest, *spec.type);
    if (structure == nullptr) {
      throw std::runtime_error("unknown data table type '" + *spec.type + "'");
    }
    if (!spec.header && structure->additionalProperties) {
      throw std::runtime_error(
          "data tables without a header cannot use type '" + *spec.type +
          "' because it allows additional properties");
    }

    rowType = struct_cpp_name(stylesheet, *structure);
    const auto symbol = cpp_symbol_name(rowType);
    decoder = spec.header ? "parse_rows_" + symbol + "(step)"
                          : "parse_positional_rows_" + symbol + "(step)";
  }

  if (!parameters.empty()) {
    parameters += ", ";
  }
  parameters += "const std::vector<" + rowType + ">& rows";

  model::Argument lowered;
  lowered.name = "rows";
  lowered.declaration =
      "const std::vector<" + rowType + "> rows = " + decoder + ";";
  arguments.push_back(std::move(lowered));
}

// Tags are named after the content type itself, the way markdown fences and
// DocStrings spell it.
std::string media_type_tag(const std::string &contentType) {
  if (contentType == "json" || contentType == "yaml") {
    return "nZucchini::" + contentType;
  }

  std::string tag;
  for (const auto character : contentType) {
    tag.push_back(std::isalnum(static_cast<unsigned char>(character)) != 0
                      ? character
                      : '_');
  }
  return tag;
}

void lower_doc_string(const StepDefinitions &manifest,
                      const Stylesheet &stylesheet, const DocStringSpec &spec,
                      std::string &parameters,
                      std::vector<model::Argument> &arguments) {
  const auto stringClass = string_class(stylesheet);
  std::string type = stringClass;
  std::string decoder =
      stringClass == "std::string"
          ? "doc_string(step).content"
          : stringClass + "((doc_string(step).content).c_str())";

  if (spec.type) {
    const auto *structure = find_struct(manifest, *spec.type);
    if (structure == nullptr) {
      throw std::runtime_error("unknown DocString type '" + *spec.type + "'");
    }
    type = struct_cpp_name(stylesheet, *structure);
    decoder = "nZucchini::MediaTypeConverter<" +
              media_type_tag(spec.contentType.value_or("json")) +
              ">::convertToJSON(doc_string(step).content).get<" + type + ">()";
  }

  if (!parameters.empty()) {
    parameters += ", ";
  }
  parameters += "const " + type + "& docString";

  model::Argument lowered;
  lowered.name = "docString";
  lowered.declaration = "const " + type + " docString = " + decoder + ";";
  arguments.push_back(std::move(lowered));
}

model::EnumDef lower_enum(const Stylesheet &stylesheet,
                          const EnumType &enumeration) {
  model::EnumDef lowered;
  lowered.cppName = enum_cpp_name(stylesheet, enumeration);
  lowered.symbolName = cpp_symbol_name(lowered.cppName);
  lowered.imported = enumeration.imported;

  for (const auto &enumCase : enumeration.cases) {
    model::EnumCaseDef loweredCase;
    loweredCase.identifier = enumCase.name;
    loweredCase.cppName = lowered.cppName + "::" + enumCase.name;
    loweredCase.values = enumCase.values;
    lowered.cases.push_back(std::move(loweredCase));
  }

  return lowered;
}

// Renders a Casing value as the C++ enum literal embedded into generated code,
// so runtime Discovery/Snippets can honor the stylesheet's snippet-casing
// preference.
std::string casing_literal(Casing casing) {
  switch (casing) {
  case Casing::SnakeCase:
    return "nZucchini::NameCasing::SnakeCase";
  case Casing::CamelCase:
    return "nZucchini::NameCasing::CamelCase";
  case Casing::PascalCase:
    return "nZucchini::NameCasing::PascalCase";
  }
  return "nZucchini::NameCasing::SnakeCase";
}

} // namespace

nZucchiniTemplates::Fixture lower(const StepDefinitions &manifest,
                                  const Stylesheet &stylesheet,
                                  const std::string &fixtureName) {
  model::Fixture fixture;
  fixture.sourceName = fixtureName;
  fixture.name = compose_fixture_name(stylesheet.fixtureNaming, fixtureName);
  fixture.stepDefinitionsJson = quote(nlohmann::json(manifest).dump());
  fixture.aroundStepName =
      apply_casing("around_step", stylesheet.aroundStepCasing);
  fixture.validateScenarioName =
      apply_casing("validate_scenario", stylesheet.validateScenarioCasing);
  fixture.snippetMethodCasing = casing_literal(stylesheet.snippetMethodCasing);
  fixture.snippetClassCasing = casing_literal(stylesheet.snippetClassCasing);

  for (const auto &type : manifest.types) {
    if (const auto *enumeration = std::get_if<EnumType>(&type)) {
      fixture.enums.push_back(lower_enum(stylesheet, *enumeration));
      continue;
    }

    const auto &structure = std::get<StructType>(type);
    model::StructDef lowered;
    lowered.cppName = struct_cpp_name(stylesheet, structure);
    lowered.symbolName = cpp_symbol_name(lowered.cppName);
    lowered.imported = structure.imported;
    lowered.additionalProperties = structure.additionalProperties;
    for (const auto &field : structure.fields) {
      if (field.type == "ignore") {
        continue;
      }
      lowered.fields.push_back(lower_field(manifest, stylesheet, field));
    }
    fixture.structs.push_back(std::move(lowered));
  }

  for (const auto &step : manifest.steps) {
    model::StepDef lowered;
    lowered.methodName =
        compose_method_name(stylesheet.methodNaming, step.methodName);
    lowered.methodLiteral = quote(lowered.methodName);
    lowered.enumCase = lowered.methodName;
    lowered.regex = quote(step.step);
    std::string parameters;
    std::size_t captureIndex = 0;
    for (const auto &argument : step.arguments) {
      // make_zucchini drops ignored captures, so they do not shift the index
      // either.
      if (argument.type == "ignore") {
        continue;
      }
      lowered.arguments.push_back(lower_capture(manifest, stylesheet, argument,
                                                captureIndex, parameters));
      ++captureIndex;
    }

    if (step.dataTable) {
      lower_data_table(manifest, stylesheet, *step.dataTable, parameters,
                       lowered.arguments);
    }
    if (step.docstring) {
      lower_doc_string(manifest, stylesheet, *step.docstring, parameters,
                       lowered.arguments);
    }

    lowered.parameters = parameters;
    fixture.steps.push_back(std::move(lowered));
  }

  return fixture;
}
} // namespace nZucchini
