#include "StylesheetParser.hpp"

#include "SchemaValidator.hpp"
#include "YamlToJson.hpp"

#include <fkYAML/node.hpp>

#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace nZucchini {
namespace {
using Node = fkyaml::node;

std::string append_path(std::string path, const std::string &key) {
  return path.empty() ? key : path + "." + key;
}

std::string append_path(std::string path, std::size_t index) {
  return path + "[" + std::to_string(index) + "]";
}

const Node *member(const Node &node, const std::string &key) {
  const auto &mapping = node.as_map();
  const auto found = mapping.find(Node(key));
  return found == mapping.end() ? nullptr : &found->second;
}

std::vector<std::string> member_names(const Node &node) {
  std::vector<std::string> names;
  for (const auto &entry : node.as_map()) {
    names.push_back(entry.first.is_string()
                        ? entry.first.get_value<std::string>()
                        : std::string());
  }
  return names;
}

// Recognized "on*" condition keys, grouped by the dimension they belong to; a
// naming block may only use keys from a single group at a time.
const std::vector<std::vector<std::string>> &type_condition_groups() {
  static const std::vector<std::vector<std::string>> groups = {
      {"onStruct", "onEnum"}};
  return groups;
}

const std::vector<std::vector<std::string>> &variable_condition_groups() {
  static const std::vector<std::vector<std::string>> groups = {
      {"onStruct", "onEnum", "onInt", "onDouble", "onBool", "onString"},
      {"onMember", "onNotMember"},
      {"onArray", "onNotArray"},
      {"onOptional", "onMandatory"},
  };
  return groups;
}

void record_parse_error(const std::string &message, Diagnostics &errors) {
  static const std::regex position("at line ([0-9]+), column ([0-9]+)");

  std::smatch match;
  if (std::regex_search(message, match, position)) {
    add_diagnostic(errors, {}, message,
                   static_cast<std::uint32_t>(std::stoul(match[1].str())),
                   static_cast<std::uint32_t>(std::stoul(match[2].str())));
    return;
  }

  add_diagnostic(errors, {}, message);
}

class StylesheetReader {
public:
  explicit StylesheetReader(Diagnostics &errors) : errors(errors) {}

  void read(const Node &root, Stylesheet &stylesheet) {
    if (!expect_mapping(root, {})) {
      return;
    }

    reject_unknown_keys(root, {},
                        {"stringClass", "hooks", "snippets", "cppConventions"});

    read_optional_string(root, {}, "stringClass", stylesheet.stringClass);

    if (const auto *hooks = member(root, "hooks")) {
      read_hooks(*hooks, "hooks", stylesheet);
    }
    if (const auto *snippets = member(root, "snippets")) {
      read_snippets(*snippets, "snippets", stylesheet);
    }
    if (const auto *conventions = member(root, "cppConventions")) {
      read_conventions(*conventions, "cppConventions", stylesheet);
    }
  }

private:
  void error(CodingPath path, std::string message) {
    add_diagnostic(errors, std::move(path), std::move(message));
  }

  bool expect_mapping(const Node &node, const CodingPath &path) {
    if (node.is_mapping()) {
      return true;
    }
    error(path, "expected a mapping");
    return false;
  }

  bool expect_sequence(const Node &node, const CodingPath &path) {
    if (node.is_sequence()) {
      return true;
    }
    error(path, "expected a sequence");
    return false;
  }

  void reject_unknown_keys(const Node &node, const CodingPath &path,
                           const std::vector<std::string> &allowed) {
    for (const auto &name : member_names(node)) {
      if (std::find(allowed.begin(), allowed.end(), name) == allowed.end()) {
        error(append_path(path, name), "unknown key");
      }
    }
  }

  bool read_string(const Node &node, const CodingPath &path,
                   std::string &value) {
    if (!node.is_string()) {
      error(path, "expected a string");
      return false;
    }
    value = node.get_value<std::string>();
    return true;
  }

  bool read_required_string(const Node &owner, const CodingPath &path,
                            const std::string &key, std::string &value) {
    const auto *node = member(owner, key);
    if (node == nullptr) {
      error(append_path(path, key), "required key is missing");
      return false;
    }
    return read_string(*node, append_path(path, key), value);
  }

  void read_optional_string(const Node &owner, const CodingPath &path,
                            const std::string &key,
                            std::optional<std::string> &value) {
    const auto *node = member(owner, key);
    if (node == nullptr || node->is_null()) {
      return;
    }
    std::string text;
    if (read_string(*node, append_path(path, key), text)) {
      value = std::move(text);
    }
  }

  std::string read_optional_string_or_empty(const Node &owner,
                                            const CodingPath &path,
                                            const std::string &key) {
    const auto *node = member(owner, key);
    if (node == nullptr || node->is_null()) {
      return {};
    }
    std::string text;
    read_string(*node, append_path(path, key), text);
    return text;
  }

  void read_casing(const Node &owner, const CodingPath &path,
                   const std::string &key, Casing &value) {
    const auto *node = member(owner, key);
    if (node == nullptr || node->is_null()) {
      return;
    }
    std::string text;
    if (!read_string(*node, append_path(path, key), text)) {
      return;
    }
    if (text == "snake_case") {
      value = Casing::SnakeCase;
    } else if (text == "camelCase") {
      value = Casing::CamelCase;
    } else if (text == "CamelCase") {
      value = Casing::PascalCase;
    } else {
      error(append_path(path, key),
            "expected one of 'snake_case', 'camelCase', 'CamelCase'");
    }
  }

  void read_hooks(const Node &node, const CodingPath &path,
                  Stylesheet &stylesheet) {
    if (!expect_mapping(node, path)) {
      return;
    }
    reject_unknown_keys(node, path, {"aroundStep", "validateScenario"});
    read_casing(node, path, "aroundStep", stylesheet.aroundStepCasing);
    read_casing(node, path, "validateScenario",
                stylesheet.validateScenarioCasing);
  }

  void read_snippets(const Node &node, const CodingPath &path,
                     Stylesheet &stylesheet) {
    if (!expect_mapping(node, path)) {
      return;
    }
    reject_unknown_keys(node, path, {"methods", "classes"});
    read_casing(node, path, "methods", stylesheet.snippetMethodCasing);
    read_casing(node, path, "classes", stylesheet.snippetClassCasing);
  }

  void read_string_sequence(const Node &node, const CodingPath &path,
                            std::vector<std::string> &values) {
    if (!expect_sequence(node, path)) {
      return;
    }
    values.clear();
    std::size_t index = 0;
    for (const auto &element : node.as_seq()) {
      std::string value;
      if (read_string(element, append_path(path, index), value)) {
        values.push_back(std::move(value));
      }
      ++index;
    }
  }

  bool read_naming_block(const Node &node, const CodingPath &path,
                         const std::vector<std::vector<std::string>> &groups,
                         NamingBlock &block) {
    if (!expect_mapping(node, path)) {
      return false;
    }

    std::vector<std::string> allowed = {"blockName", "default"};
    for (const auto &group : groups) {
      allowed.insert(allowed.end(), group.begin(), group.end());
    }
    reject_unknown_keys(node, path, allowed);

    if (!read_required_string(node, path, "blockName", block.blockName)) {
      return false;
    }
    block.defaultValue = read_optional_string_or_empty(node, path, "default");

    int matchedGroup = -1;
    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
      for (const auto &key : groups[groupIndex]) {
        if (member(node, key) == nullptr) {
          continue;
        }
        if (matchedGroup != -1 &&
            matchedGroup != static_cast<int>(groupIndex)) {
          error(path,
                "a naming block may only use conditions from a single group "
                "(kind, member, list, or optional)");
          return false;
        }
        matchedGroup = static_cast<int>(groupIndex);
      }
    }

    if (matchedGroup == -1) {
      error(path, "naming block has no 'on*' condition");
      return false;
    }

    for (const auto &key : groups[static_cast<std::size_t>(matchedGroup)]) {
      if (const auto *valueNode = member(node, key)) {
        std::string value;
        if (read_string(*valueNode, append_path(path, key), value)) {
          block.cases.emplace_back(key, value);
        }
      }
    }

    return true;
  }

  void read_naming_blocks(const Node &node, const CodingPath &path,
                          const std::vector<std::vector<std::string>> &groups,
                          std::vector<NamingBlock> &blocks) {
    if (!expect_sequence(node, path)) {
      return;
    }
    blocks.clear();
    std::set<std::string> seenNames;
    std::size_t index = 0;
    for (const auto &element : node.as_seq()) {
      const auto blockPath = append_path(path, index);
      ++index;

      NamingBlock block;
      if (!read_naming_block(element, blockPath, groups, block)) {
        continue;
      }
      if (!seenNames.insert(block.blockName).second) {
        error(append_path(blockPath, "blockName"), "duplicate blockName");
        continue;
      }
      blocks.push_back(std::move(block));
    }
  }

  void read_naming_rule(const Node &node, const CodingPath &path,
                        const std::string &verbatimKeyword,
                        const std::vector<std::vector<std::string>> &groups,
                        NamingRule &rule) {
    if (!expect_mapping(node, path)) {
      return;
    }
    reject_unknown_keys(node, path, {"blocks", "nameIt"});

    rule = NamingRule(verbatimKeyword);

    if (const auto *blocks = member(node, "blocks")) {
      read_naming_blocks(*blocks, append_path(path, "blocks"), groups,
                         rule.blocks);
    }
    if (const auto *nameIt = member(node, "nameIt")) {
      read_string_sequence(*nameIt, append_path(path, "nameIt"), rule.nameIt);
    }
  }

  void read_method_naming_rule(const Node &node, const CodingPath &path,
                               NamingRule &rule) {
    if (!expect_mapping(node, path)) {
      return;
    }
    reject_unknown_keys(node, path, {"nameIt"});

    rule = NamingRule("methodName");

    if (const auto *nameIt = member(node, "nameIt")) {
      read_string_sequence(*nameIt, append_path(path, "nameIt"), rule.nameIt);
    }
  }

  void read_conventions(const Node &node, const CodingPath &path,
                        Stylesheet &stylesheet) {
    if (!expect_mapping(node, path)) {
      return;
    }
    reject_unknown_keys(node, path, {"classes", "types", "methods", "variables"});

    if (const auto *classes = member(node, "classes")) {
      if (expect_mapping(*classes, append_path(path, "classes"))) {
        reject_unknown_keys(*classes, append_path(path, "classes"),
                            {"fixture", "interface"});
        if (const auto *fixture = member(*classes, "fixture")) {
          read_method_naming_rule(*fixture,
                                  append_path(path, "classes.fixture"),
                                  stylesheet.fixtureNaming);
        }
        if (const auto *interface = member(*classes, "interface")) {
          read_method_naming_rule(*interface,
                                  append_path(path, "classes.interface"),
                                  stylesheet.fixtureInterfaceNaming);
        }
      }
    }

    if (const auto *types = member(node, "types")) {
      read_naming_rule(*types, append_path(path, "types"), "typeName",
                       type_condition_groups(), stylesheet.typeNaming);
    }
    if (const auto *methods = member(node, "methods")) {
      read_method_naming_rule(*methods, append_path(path, "methods"),
                              stylesheet.methodNaming);
    }
    if (const auto *variables = member(node, "variables")) {
      read_naming_rule(*variables, append_path(path, "variables"),
                       "variableName", variable_condition_groups(),
                       stylesheet.variableNaming);
    }
  }

  Diagnostics &errors;
};
} // namespace

bool parse_stylesheet(const std::string &yaml, Stylesheet &stylesheet,
                      Diagnostics &errors) {
  stylesheet = Stylesheet();
  errors.clear();

  Node root;
  try {
    root = Node::deserialize(yaml);
  } catch (const fkyaml::exception &failure) {
    record_parse_error(failure.what(), errors);
    return false;
  }

  nlohmann::json document;
  std::string conversionError;
  if (!yaml_to_json(yaml, document, conversionError)) {
    record_parse_error(conversionError, errors);
    return false;
  }

  if (!validate_against_stylesheet_schema(document, errors)) {
    return false;
  }

  StylesheetReader(errors).read(root, stylesheet);

  if (!errors.empty()) {
    stylesheet = Stylesheet();
    return false;
  }
  return true;
}

bool parse_stylesheet_from_file(const std::string &filePath,
                                Stylesheet &stylesheet, Diagnostics &errors) {
  stylesheet = Stylesheet();
  errors.clear();

  std::ifstream file(filePath);
  if (!file) {
    add_diagnostic(errors, {}, "cannot open '" + filePath + "'");
    return false;
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  return parse_stylesheet(contents.str(), stylesheet, errors);
}
} // namespace nZucchini
