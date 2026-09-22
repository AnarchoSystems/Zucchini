#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nZucchini {
enum class Casing { SnakeCase, CamelCase, PascalCase };

std::string to_string(Casing casing);
std::string apply_casing(const std::string &text, Casing casing);

// The kind a variable's declared type falls into, used by the "kind" condition
// group (onStruct/onEnum/onInt/onDouble/onBool/onString) in a variable naming
// block.
enum class VariableKind { Struct, Enum, Int, Double, Bool, String };

// One block of a naming rule: exactly one condition group (kind, member, list,
// optional) should be represented among `cases`; the first case whose condition
// holds wins, else `defaultValue`.
struct NamingBlock {
  NamingBlock() = default;

  std::string blockName;
  std::vector<std::pair<std::string, std::string>> cases;
  std::string defaultValue;
};

// blocks compose into `nameIt`, an ordered list of tokens: the section's
// verbatim keyword (typeName/methodName/variableName), a declared blockName, or
// otherwise a literal.
struct NamingRule {
  NamingRule() = default;
  explicit NamingRule(std::string verbatimToken)
      : nameIt({std::move(verbatimToken)}) {}

  std::vector<NamingBlock> blocks;
  std::vector<std::string> nameIt;
};

struct Stylesheet {
  Stylesheet()
      : typeNaming("typeName"), fixtureNaming("fixtureName"),
        methodNaming("methodName"),
        variableNaming("variableName") {}

  std::optional<std::string> stringClass;
  Casing aroundStepCasing = Casing::SnakeCase;
  Casing validateScenarioCasing = Casing::SnakeCase;
  Casing snippetMethodCasing = Casing::SnakeCase;
  Casing snippetClassCasing = Casing::PascalCase;
  NamingRule typeNaming;
  NamingRule fixtureNaming;
  NamingRule methodNaming;
  NamingRule variableNaming;
};

bool operator==(const NamingBlock &lhs, const NamingBlock &rhs);
bool operator==(const NamingRule &lhs, const NamingRule &rhs);
bool operator==(const Stylesheet &lhs, const Stylesheet &rhs);

void to_json(nlohmann::json &json, const NamingBlock &block);
void to_json(nlohmann::json &json, const NamingRule &rule);
void to_json(nlohmann::json &json, const Stylesheet &stylesheet);

// The naming engine: composes a C++ identifier out of a NamingRule and the
// manifest-verbatim name.
std::string compose_type_name(const NamingRule &rule,
                              const std::string &typeName, bool isEnum);
std::string compose_fixture_name(const NamingRule &rule,
                                 const std::string &fixtureName);
std::string compose_method_name(const NamingRule &rule,
                                const std::string &methodName);
std::string compose_variable_name(const NamingRule &rule,
                                  const std::string &variableName,
                                  VariableKind kind, bool isMember, bool isList,
                                  bool isOptional);
} // namespace nZucchini
