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

std::string evaluate_block(const NamingBlock &block,
                           const std::set<std::string> &activeConditions) {
  for (const auto &[condition, value] : block.cases) {
    if (activeConditions.count(condition) != 0) {
      return value;
    }
  }
  return block.defaultValue;
}

std::string compose(const NamingRule &rule, const std::string &verbatimKeyword,
                    const std::string &verbatimValue,
                    const std::set<std::string> &activeConditions) {
  std::string result;
  for (const auto &token : rule.nameIt) {
    if (token == verbatimKeyword) {
      result += verbatimValue;
      continue;
    }
    if (const auto *block = find_block(rule, token)) {
      result += evaluate_block(*block, activeConditions);
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
    return "CamelCase";
  }
  return "snake_case";
}

bool operator==(const NamingBlock &lhs, const NamingBlock &rhs) {
  return lhs.blockName == rhs.blockName && lhs.cases == rhs.cases &&
         lhs.defaultValue == rhs.defaultValue;
}

bool operator==(const NamingRule &lhs, const NamingRule &rhs) {
  return lhs.blocks == rhs.blocks && lhs.nameIt == rhs.nameIt;
}

bool operator==(const Stylesheet &lhs, const Stylesheet &rhs) {
  return lhs.stringClass == rhs.stringClass &&
         lhs.aroundStepCasing == rhs.aroundStepCasing &&
         lhs.validateScenarioCasing == rhs.validateScenarioCasing &&
         lhs.snippetMethodCasing == rhs.snippetMethodCasing &&
         lhs.snippetClassCasing == rhs.snippetClassCasing &&
         lhs.typeNaming == rhs.typeNaming &&
         lhs.fixtureNaming == rhs.fixtureNaming &&
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
}

void to_json(nlohmann::json &json, const Stylesheet &stylesheet) {
  json = nlohmann::json{
      {"stringClass", stylesheet.stringClass
                          ? nlohmann::json(*stylesheet.stringClass)
                          : nlohmann::json()},
      {"hooks",
       {{"aroundStep", to_string(stylesheet.aroundStepCasing)},
        {"validateScenario", to_string(stylesheet.validateScenarioCasing)}}},
      {"snippets",
       {{"methods", to_string(stylesheet.snippetMethodCasing)},
        {"classes", to_string(stylesheet.snippetClassCasing)}}},
      {"cppConventions",
      {{"fixture", stylesheet.fixtureNaming},
       {"types", stylesheet.typeNaming},
        {"methods", stylesheet.methodNaming},
        {"variables", stylesheet.variableNaming}}}};
}

std::string compose_type_name(const NamingRule &rule,
                              const std::string &typeName, bool isEnum) {
  const std::set<std::string> active = {isEnum ? "onEnum" : "onStruct"};
  return compose(rule, "typeName", typeName, active);
}

std::string compose_fixture_name(const NamingRule &rule,
                                 const std::string &fixtureName) {
  return compose(rule, "fixtureName", fixtureName, {});
}

std::string compose_method_name(const NamingRule &rule,
                                const std::string &methodName) {
  return compose(rule, "methodName", methodName, {});
}

std::string compose_variable_name(const NamingRule &rule,
                                  const std::string &variableName,
                                  VariableKind kind, bool isMember, bool isList,
                                  bool isOptional) {
  const std::set<std::string> active = {
      kind_condition(kind),
      isMember ? "onMember" : "onNotMember",
      isList ? "onArray" : "onNotArray",
      isOptional ? "onOptional" : "onMandatory",
  };
  return compose(rule, "variableName", variableName, active);
}
} // namespace nZucchini
