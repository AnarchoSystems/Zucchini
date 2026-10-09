#include "Zucchini/YamlDocument.hpp"
#include <libfyaml/libfyaml-core.h>
#include <algorithm>
#include <cmath>
#include <regex>
#include <stdexcept>

namespace nZucchini {
namespace {
std::string scalar(fy_node* node) {
  std::size_t size = 0;
  const auto* value = fy_node_get_scalar(node, &size);
  return value ? std::string(value, size) : std::string();
}

nlohmann::json convert(fy_node* node) {
  if (!node || fy_node_is_null(node)) {
    return nullptr;
  }
  if (fy_node_is_sequence(node)) {
    auto result = nlohmann::json::array();
    void* iterator = nullptr;
    while (auto* child = fy_node_sequence_iterate(node, &iterator)) {
      result.push_back(convert(child));
    }
    return result;
  }
  if (fy_node_is_mapping(node)) {
    auto result = nlohmann::json::object();
    void* iterator = nullptr;
    while (auto* pair = fy_node_mapping_iterate(node, &iterator)) {
      auto* key = fy_node_pair_key(pair);
      if (!fy_node_is_scalar(key)) {
        throw std::runtime_error("YAML mapping keys must be scalars");
      }
      result[scalar(key)] = convert(fy_node_pair_value(pair));
    }
    return result;
  }

  const auto text = scalar(node);
  std::size_t tagSize = 0;
  const auto* tagData = fy_node_get_tag(node, &tagSize);
  const auto tag = tagData ? std::string(tagData, tagSize) : std::string();
  const auto explicitString = tag == "tag:yaml.org,2002:str" || tag == "!";
  if (explicitString || (tag.empty() && fy_node_get_style(node) != FYNS_PLAIN)) {
    return text;
  }
  if (text.empty() || text == "~" || text == "null" || text == "Null" ||
      text == "NULL" || tag == "tag:yaml.org,2002:null") {
    return nullptr;
  }
  if (text == "true" || text == "True" || text == "TRUE") {
    return true;
  }
  if (text == "false" || text == "False" || text == "FALSE") {
    return false;
  }
  static const std::regex integer(R"(^[+-]?(?:[0-9]+|0x[0-9a-fA-F]+|0o[0-7]+)$)");
  static const std::regex number(R"(^[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$)");
  if (std::regex_match(text, integer)) {
    const auto sign = text.front() == '+' || text.front() == '-' ? 1u : 0u;
    if (text.compare(sign, 2, "0o") == 0) {
      const auto magnitude = text.substr(0, sign) + text.substr(sign + 2);
      return std::stoll(magnitude, nullptr, 8);
    }
    return std::stoll(text, nullptr, text.compare(sign, 2, "0x") == 0 ? 16 : 10);
  }
  if (std::regex_match(text, number)) {
    return std::stod(text);
  }
  if (text == ".inf" || text == ".Inf" || text == ".INF" ||
      text == "+.inf" || text == "-.inf" || text == ".nan" ||
      text == ".NaN" || text == ".NAN") {
    return nullptr; // JSON cannot represent non-finite YAML numbers.
  }
  if (!tag.empty() && tag != "tag:yaml.org,2002:str") {
    throw std::runtime_error("unsupported YAML scalar tag '" + tag + "'");
  }
  return text;
}
}

YamlDocument::YamlDocument(std::string input, bool resolve)
    : source(std::move(input)) {
  fy_diag_cfg diagnostics{};
  fy_diag_cfg_default(&diagnostics);
  diagnostics.fp = nullptr;
  diagnostics.output_fn = [](fy_diag*, void*, const char*, std::size_t) {};
  diag = fy_diag_create(&diagnostics);
  if (!diag) {
    throw std::runtime_error("cannot create YAML diagnostics");
  }
  fy_diag_set_collect_errors(diag, true);
  fy_parse_cfg config{};
  config.diag = diag;
  config.flags = static_cast<fy_parse_cfg_flags>(
      FYPCF_QUIET | FYPCF_COLLECT_DIAG | FYPCF_DEFAULT_VERSION_1_2 |
      (resolve ? FYPCF_RESOLVE_DOCUMENT : 0));
  document = fy_document_build_from_string(&config, source.data(), source.size());
  if (!document || fy_diag_got_error(diag)) {
    void* iterator = nullptr;
    const auto* diagnostic = fy_diag_errors_iterate(diag, &iterator);
    const auto message = diagnostic
        ? std::string(diagnostic->msg) + " (at line " +
              std::to_string(diagnostic->line) + ", column " +
              std::to_string(diagnostic->column) + ")"
        : std::string("cannot parse YAML document");
    fy_document_destroy(document);
    document = nullptr;
    fy_diag_destroy(diag);
    diag = nullptr;
    throw std::runtime_error(message);
  }
}
YamlDocument::~YamlDocument() {
  fy_document_destroy(document);
  fy_diag_destroy(diag);
}
fy_node* YamlDocument::root() const { return fy_document_root(document); }
nlohmann::json YamlDocument::json() const { return convert(root()); }

bool yaml_to_json(const std::string& yaml, nlohmann::json& json,
                  std::string& error) {
  try {
    json = YamlDocument(yaml).json();
    error.clear();
    return true;
  } catch (const std::exception& failure) {
    error = failure.what();
    return false;
  }
}
}
