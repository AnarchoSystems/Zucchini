#include "Stylesheet.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace nZucchini {
namespace {
std::vector<std::string> split_words(const std::string &text) {
  std::vector<std::string> words;
  std::string current;
  const auto flush = [&]() {
    if (!current.empty()) {
      words.push_back(current);
      current.clear();
    }
  };

  for (const auto character : text) {
    const auto value = static_cast<unsigned char>(character);
    if (std::isalnum(value) == 0) {
      flush();
      continue;
    }
    if (!current.empty()) {
      const auto previous = static_cast<unsigned char>(current.back());
      if ((std::islower(previous) != 0 && std::isupper(value) != 0) ||
          ((std::isalpha(previous) != 0) != (std::isalpha(value) != 0))) {
        flush();
      }
    }
    current.push_back(character);
  }
  flush();
  return words;
}

const NamingBlock *find_block(const NamingRule &rule, const std::string &name) {
  for (const auto &block : rule.blocks) {
    if (block.blockName == name) {
      return &block;
    }
  }
  return nullptr;
}

const char *kind_condition(VariableKind kind);

std::string evaluate_block(
    const NamingRule &rule, const NamingBlock &block,
    const std::set<std::string> &activeConditions,
    std::optional<VariableKind> mapKeyType = {},
    std::optional<VariableKind> mapValueType = {}) {
  for (const auto &[condition, value] : block.cases) {
    if (activeConditions.count(condition) == 0) {
      continue;
    }
    if (condition == "onMapKeyType" || condition == "onMapValueType") {
      const auto kind = condition == "onMapKeyType" ? mapKeyType : mapValueType;
      const auto *referencedBlock = find_block(rule, value);
      if (!kind || referencedBlock == nullptr) {
        return {};
      }
      return evaluate_block(rule, *referencedBlock, {kind_condition(*kind)});
    }
    return value;
  }
  return block.defaultValue;
}

std::string compose(const NamingRule &rule, const std::string &verbatimKeyword,
                    const std::string &verbatimValue,
                    const std::set<std::string> &activeConditions,
                    std::optional<VariableKind> mapKeyType = {},
                    std::optional<VariableKind> mapValueType = {}) {
  const auto value = rule.casing ? apply_casing(verbatimValue, *rule.casing)
                                 : verbatimValue;
  std::string result;
  for (const auto &token : rule.nameIt) {
    if (token == verbatimKeyword) {
      result += value;
      continue;
    }
    if (const auto *block = find_block(rule, token)) {
      result += evaluate_block(rule, *block, activeConditions, mapKeyType,
                               mapValueType);
      continue;
    }
    result += token;
  }
  return result;
}

const char *kind_condition(VariableKind kind) {
  switch (kind) {
  case VariableKind::Struct:
    return "onStruct";
  case VariableKind::Enum:
    return "onEnum";
  case VariableKind::Int:
    return "onInt";
  case VariableKind::Double:
    return "onDouble";
  case VariableKind::Bool:
    return "onBool";
  case VariableKind::String:
    return "onString";
  }
  return "onString";
}
} // namespace

std::string apply_casing(const std::string &text, Casing casing) {
  const auto words = split_words(text);
  std::string result;
  for (std::size_t index = 0; index < words.size(); ++index) {
    auto word = words[index];
    std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (casing == Casing::SnakeCase) {
      if (index != 0) {
        result += '_';
      }
      result += word;
      continue;
    }
    if ((casing == Casing::PascalCase || index != 0) && !word.empty()) {
      word[0] =
          static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
    }
    result += word;
  }
  return result;
}

std::string to_string(Casing casing) {
  switch (casing) {
  case Casing::SnakeCase:
    return "snake_case";
  case Casing::CamelCase:
    return "camelCase";
  case Casing::PascalCase:
    return "PascalCase";
  }
  return "snake_case";
}

bool operator==(const NamingBlock &lhs, const NamingBlock &rhs) {
  return lhs.blockName == rhs.blockName && lhs.cases == rhs.cases &&
         lhs.defaultValue == rhs.defaultValue;
}

bool operator==(const NamingRule &lhs, const NamingRule &rhs) {
  return lhs.blocks == rhs.blocks && lhs.nameIt == rhs.nameIt &&
         lhs.casing == rhs.casing;
}

bool operator==(const StringClass &lhs, const StringClass &rhs) {
  return lhs.name == rhs.name && lhs.cStrMethod == rhs.cStrMethod;
}

bool operator==(const Stylesheet &lhs, const Stylesheet &rhs) {
  return lhs.stringClass == rhs.stringClass &&
         lhs.commonIncludes == rhs.commonIncludes &&
         lhs.mapBaseNameIsSingular == rhs.mapBaseNameIsSingular &&
         lhs.typeNaming == rhs.typeNaming &&
         lhs.methodNaming == rhs.methodNaming &&
         lhs.variableNaming == rhs.variableNaming;
}

void to_json(nlohmann::json &json, const NamingBlock &block) {
  json = nlohmann::json{{"blockName", block.blockName},
                        {"default", block.defaultValue}};
  for (const auto &[condition, value] : block.cases) {
    json[condition] = value;
  }
}

void to_json(nlohmann::json &json, const NamingRule &rule) {
  json = nlohmann::json{{"blocks", rule.blocks}, {"nameIt", rule.nameIt}};
  if (rule.casing) {
    json["casing"] = to_string(*rule.casing);
  }
}

void to_json(nlohmann::json &json, const Stylesheet &stylesheet) {
  auto variables = nlohmann::json(stylesheet.variableNaming);
  variables["mapBaseNameIsSingular"] = stylesheet.mapBaseNameIsSingular;
  json = nlohmann::json{
      {"stringClass", stylesheet.stringClass
              ? nlohmann::json{{"name", stylesheet.stringClass->name},
                   {"cStrMethod", stylesheet.stringClass->cStrMethod}}
              : nlohmann::json()},
       {"commonIncludes", stylesheet.commonIncludes},
      {"cppConventions",
       {{"types", stylesheet.typeNaming},
        {"methods", stylesheet.methodNaming},
        {"variables", std::move(variables)}}}};
}

std::string compose_type_name(const NamingRule &rule,
                              const std::string &typeName, bool isEnum) {
  const std::set<std::string> active = {isEnum ? "onEnum" : "onStruct"};
  return compose(rule, "typeName", typeName, active);
}

std::string compose_class_name(const NamingRule &rule,
                               const std::string &className) {
  return compose(rule, "typeName", className, {"onClass"});
}

std::string compose_method_name(const NamingRule &rule,
                                const std::string &methodName, bool isHook) {
  return compose(rule, "methodName", methodName,
                 {isHook ? "onHook" : "onStepDefinition"});
}

std::string compose_variable_name(const NamingRule &rule,
                                  const std::string &variableName,
                                  VariableKind kind, bool isMember,
                                  bool isArray, bool isOptional,
                                  std::optional<VariableKind> mapKeyType,
                                  std::optional<VariableKind> mapValueType) {
  auto active = std::set<std::string>{
      isMember ? "onMember" : "onNotMember",
      mapKeyType || mapValueType ? "onMap" : "onNotMap",
      isArray ? "onArray" : "onNotArray",
      isOptional ? "onOptional" : "onMandatory",
  };
  if (!mapKeyType && !mapValueType) {
    active.insert(kind_condition(kind));
  }
  if (mapKeyType) {
    active.insert("onMapKeyType");
  }
  if (mapValueType) {
    active.insert("onMapValueType");
  }
  return compose(rule, "variableName", variableName, active, mapKeyType,
                 mapValueType);
}
} // namespace nZucchini
