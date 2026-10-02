#include "Lowering.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
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
                           /*isEnum=*/true, /*applyCasing=*/false);
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
                           /*isEnum=*/false, /*applyCasing=*/false);
}

std::string cpp_symbol_name(const std::string &type) {
  const auto pos = type.rfind("::");
  return pos == std::string::npos ? type : type.substr(pos + 2);
}

std::string string_class(const Stylesheet &stylesheet) {
  return stylesheet.stringClass ? stylesheet.stringClass->name : "std::string";
}

std::string string_cstr_method(const Stylesheet &stylesheet) {
  return stylesheet.stringClass ? stylesheet.stringClass->cStrMethod : "c_str";
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
    const auto isArray = field.type == "list";
  const auto kind = kind_of_type(
      manifest, isArray ? field.content.value_or("string") : field.type);
  lowered.cppName =
      compose_variable_name(stylesheet.variableNaming, field.name, kind,
                /*isMember=*/true, isArray, field.optional, {}, {},
                /*applyCasing=*/false);
  lowered.headers = field.headers.empty() ? std::vector<std::string>{field.name}
                                          : field.headers;
  lowered.header = lowered.headers.front();
  lowered.isOptional = field.optional;
  lowered.hasDefault = false;
  lowered.isString = false;
  lowered.isStringList = false;

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
      lowered.isStringList = stringClass != "std::string";
      const auto listType = "std::vector<" + stringClass + ">";
      lowered.declType = field.optional ? "std::optional<" + listType + ">" : listType;
      lowered.valueType = listType;
      if (stringClass == "std::string") {
        lowered.reader = field.optional
                             ? "cell_or(row, " + headers + ", \"\").empty() ? std::nullopt : std::optional<" + listType + ">(" + split + ")"
                             : split;
      } else {
        // Build elements via .c_str() so string classes that only take a const
        // char* work too.
        const auto reader =
          "[&]{ std::vector<" + stringClass +
          "> elements; for (const auto& part : " + split +
          ") elements.emplace_back(part.c_str()); return elements; }()";
        lowered.reader = field.optional
                   ? "cell_or(row, " + headers + ", \"\").empty() ? std::nullopt : std::optional<" + listType + ">(" + reader + ")"
                   : reader;
      }
      return lowered;
    }

    const auto *enumeration = find_enum(manifest, content);
    if (enumeration == nullptr) {
      throw std::runtime_error("list content type '" + content +
                               "' is not a declared enum");
    }

    const auto element = enum_cpp_name(stylesheet, *enumeration);
    const auto listType = "std::vector<" + element + ">";
    lowered.declType = field.optional ? "std::optional<" + listType + ">" : listType;
    lowered.valueType = listType;
    const auto reader = "parse_list_" + cpp_symbol_name(element) + "(" + split + ")";
    lowered.reader = field.optional
               ? "cell_or(row, " + headers + ", \"\").empty() ? std::nullopt : std::optional<" + listType + ">("
                   + reader + ")"
               : reader;
    return lowered;
  }

  const auto type = cpp_type(manifest, stylesheet, field.type);
  lowered.valueType = type.declType;
  lowered.isString = (field.type.empty() || field.type == "string") &&
                     string_class(stylesheet) != "std::string";
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
      /*isMember=*/false, /*isArray=*/false, /*isOptional=*/false, {}, {},
      /*applyCasing=*/false);
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
  lowered.valueType = type.declType;
  lowered.decoder = decoded;
  lowered.initializer = name + "(" + decoded + ")";
  return lowered;
}

void lower_data_table(const StepDefinitions &manifest,
                      const Stylesheet &stylesheet, const DataTableSpec &spec,
                      std::string &parameters,
                      std::vector<model::Argument> &arguments) {
  const auto stringClass = string_class(stylesheet);
  std::string rowType = "std::map<" + stringClass + ", " + stringClass + ">";
  std::string decoder = "nZucchini::DataTable::from_step(step).dictionary_rows<" +
                        stringClass + ">()";

  if (!spec.header) {
    rowType = "std::vector<" + stringClass + ">";
    decoder = "nZucchini::DataTable::from_step(step).positional_rows<" +
              stringClass + ">()";
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
  lowered.valueType = "std::vector<" + rowType + ">";
  lowered.decoder = decoder;
  lowered.initializer = "rows(" + decoder + ")";
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
  lowered.valueType = type;
  lowered.decoder = decoder;
  lowered.initializer = "docString(" + decoder + ")";
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
    loweredCase.identifier = enumeration.prefix + enumCase.name;
    loweredCase.cppName = lowered.cppName + "::" + enumeration.prefix + enumCase.name;
    loweredCase.values = enumCase.values;
    lowered.cases.push_back(std::move(loweredCase));
  }

  return lowered;
}

// Renders a Casing value as the C++ enum literal used by runtime suggestions.
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

std::string compose_interface_name(const NamingRule &rule,
                                   const std::string &fixtureName) {
  return compose_class_name(rule, "I" + fixtureName,
                            /*applyCasing=*/false);
}

} // namespace

nZucchiniTemplates::Fixture lower(const StepDefinitions &manifest,
                                  const Stylesheet &stylesheet,
                                  const std::string &fixtureName) {
  ParsedManifest parsed;
  parsed.definitions = manifest;
  parsed.stepTags.resize(manifest.steps.size());
  return lower(parsed, stylesheet, fixtureName);
}

nZucchiniTemplates::Fixture lower(const ParsedManifest &parsed,
                                  const Stylesheet &stylesheet,
                                  const std::string &fixtureName) {
  const auto &manifest = parsed.definitions;
  if (parsed.stepTags.size() != manifest.steps.size()) {
    throw std::invalid_argument(
        "parsed manifest step-tag metadata does not match its step count");
  }
  model::Fixture fixture;
  fixture.sourceName = fixtureName;
  fixture.namespaceName = fixtureName;
    fixture.name = compose_class_name(stylesheet.typeNaming, fixtureName,
                     /*applyCasing=*/false);
    fixture.interfaceName =
      compose_interface_name(stylesheet.typeNaming, fixtureName);
    fixture.stepMethodName =
      compose_type_name(stylesheet.typeNaming, "StepMethod", true);
    fixture.scenarioContextName =
      compose_type_name(stylesheet.typeNaming, "ScenarioContext", false);
    fixture.stepViewName =
      compose_type_name(stylesheet.typeNaming, "StepView", false);
    fixture.stepContextName =
      compose_type_name(stylesheet.typeNaming, "StepContext", false);
    fixture.stepTagName =
      compose_type_name(stylesheet.typeNaming, "StepTag", true);
    fixture.stepTagsName =
      compose_class_name(stylesheet.typeNaming, "StepTags");
    fixture.exposeStepMethodWrapper = fixture.stepMethodName != "step_method";
    fixture.exposeStepTagsWrapper = fixture.stepTagsName != "step_tags";
    fixture.stringCStrMethod = string_cstr_method(stylesheet);
    fixture.stringClassName = string_class(stylesheet);
    fixture.commonIncludes = stylesheet.commonIncludes;
  auto loweredManifest = nlohmann::json(manifest);
    fixture.aroundStepName = compose_method_name(stylesheet.methodNaming,
                             "around_step", true);
    fixture.validateScenarioName = compose_method_name(
        stylesheet.methodNaming, "validate_scenario", true);
      fixture.methodsCasing = casing_literal(
      stylesheet.methodNaming.casing.value_or(Casing::SnakeCase));
      fixture.typesCasing = casing_literal(
      stylesheet.typeNaming.casing.value_or(Casing::PascalCase));
      fixture.variablesCasing = casing_literal(
        stylesheet.variableNaming.casing.value_or(Casing::CamelCase));

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
    if (structure.additionalProperties) {
      if (structure.imported) {
        lowered.additionalPropertiesName = "additionalProperties";
      } else {
        const auto mapBaseName = stylesheet.mapBaseNameIsSingular
                                     ? "additionalProperty"
                                     : "additionalProperties";
        lowered.additionalPropertiesName = compose_variable_name(
            stylesheet.variableNaming, mapBaseName, VariableKind::String,
            /*isMember=*/true, /*isArray=*/false, /*isOptional=*/false,
          VariableKind::String, VariableKind::String);
      }
    }
    for (const auto &field : structure.fields) {
      if (field.type == "ignore") {
        continue;
      }
      lowered.fields.push_back(lower_field(manifest, stylesheet, field));
    }
    fixture.structs.push_back(std::move(lowered));
  }

  for (std::size_t stepIndex = 0; stepIndex < manifest.steps.size();
       ++stepIndex) {
    const auto &step = manifest.steps[stepIndex];
    model::StepDef lowered;
    lowered.methodName =
      compose_method_name(stylesheet.methodNaming, step.methodName,
                /*isHook=*/false, /*applyCasing=*/false);
    lowered.methodLiteral = quote(lowered.methodName);
    lowered.enumCase = lowered.methodName;
    lowered.regex = quote(step.step);
    if (stepIndex < parsed.stepTags.size()) {
      lowered.tags = parsed.stepTags[stepIndex];
    }
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
    lowered.hasArgs = !lowered.arguments.empty();
    if (lowered.hasArgs) {
      lowered.argsTypeName = compose_args_type_name(
          stylesheet.typeNaming, step.methodName + "Args",
          /*applyCasing=*/false);
      std::set<std::string> argumentNames;
      for (const auto &argument : lowered.arguments) {
        if (!argumentNames.insert(argument.name).second) {
          throw std::runtime_error("step '" + step.methodName +
                                  "' has duplicate argument name '" +
                                  argument.name + "'");
        }
      }
    }
    fixture.steps.push_back(std::move(lowered));
  }

  std::vector<std::string> tagNames;
  for (const auto &step : fixture.steps) {
    for (const auto &tag : step.tags) {
      if (std::find(tagNames.begin(), tagNames.end(), tag) == tagNames.end()) {
        tagNames.push_back(tag);
      }
    }
  }
  fixture.tagCount = std::to_string(tagNames.size());
  fixture.hasTags = !tagNames.empty();

  std::map<std::string, std::string> typeDeclarations;
  const auto registerType = [&typeDeclarations](const std::string &name,
                                                const std::string &owner) {
    if (name.empty() || name.find("::") != std::string::npos) {
      return;
    }
    const auto [existing, inserted] = typeDeclarations.emplace(name, owner);
    if (!inserted) {
      throw std::runtime_error("C++ type '" + name + "' for " + owner +
                               " collides with " + existing->second);
    }
  };

  registerType(fixture.name, "the fixture class");
  registerType(fixture.interfaceName, "the fixture interface");
  registerType(fixture.interfaceName + "Interface", "the fixture interface base");
  registerType(fixture.interfaceName + "Decorator", "the fixture decorator");
  registerType(fixture.interfaceName + "DefaultThrowing",
               "the default throwing fixture");
  registerType(fixture.stepMethodName, "the generated StepMethod enum");
  registerType(fixture.scenarioContextName, "the ScenarioContext alias");
  registerType(fixture.stepViewName, "the StepView alias");
  registerType(fixture.stepContextName, "the StepContext alias");
  registerType("Row", "the generated Row alias");
  if (fixture.hasTags) {
    registerType(fixture.stepTagName, "the generated StepTag enum");
    registerType(fixture.stepTagsName, "the generated StepTags alias");
  }
  for (const auto &enumeration : fixture.enums) {
    if (!enumeration.imported) {
      registerType(enumeration.cppName,
                   "manifest enum '" + enumeration.symbolName + "'");
    }
  }
  for (const auto &structure : fixture.structs) {
    if (!structure.imported) {
      registerType(structure.cppName,
                   "manifest struct '" + structure.symbolName + "'");
    }
  }
  for (const auto &step : fixture.steps) {
    if (step.hasArgs) {
      registerType(step.argsTypeName,
                   "Args type for step method '" + step.methodName + "'");
    }
  }

  std::map<std::string, std::string> tagStepsNames;
  for (const auto &tagName : tagNames) {
    model::TagDef tag;
    tag.name = tagName;
    tag.enumCase = tagName;
    tag.stepsTypeName = compose_type_name(
      stylesheet.typeNaming, tagName + "Steps", /*isEnum=*/true,
      /*applyCasing=*/false);
    const auto [existing, inserted] =
        tagStepsNames.emplace(tag.stepsTypeName, tagName);
    if (!inserted) {
      throw std::runtime_error("tag labels '" + existing->second + "' and '" +
                               tagName + "' both generate step enum '" +
                               tag.stepsTypeName + "'");
    }
    const auto existingType = typeDeclarations.find(tag.stepsTypeName);
    if (existingType != typeDeclarations.end()) {
      throw std::runtime_error("tag label '" + tagName + "' generates step enum '" +
                               tag.stepsTypeName + "' which collides with " +
                               existingType->second);
    }
    typeDeclarations.emplace(tag.stepsTypeName, "tag label '" + tagName + "'");
    for (const auto &step : fixture.steps) {
      const auto found = std::find(step.tags.begin(), step.tags.end(), tagName);
      tag.methods.push_back(
          {step.enumCase, found != step.tags.end()});
    }
    fixture.tags.push_back(std::move(tag));
  }

  for (std::size_t index = 0; index < fixture.steps.size(); ++index) {
    loweredManifest["steps"][index]["methodName"] =
        fixture.steps[index].methodName;
  }
  fixture.stepDefinitionsJson = quote(loweredManifest.dump());

  return fixture;
}
} // namespace nZucchini
