#include "ManifestUpdater.hpp"
#include "ManifestParser.hpp"
#include <Zucchini/YamlDocument.hpp>
#include <libfyaml/libfyaml-core.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace nZucchini {
namespace {
using Json = nlohmann::json;
struct Edit {
  std::size_t position;
  std::string text;
  std::size_t length = 0;
};
class ManifestLock {
public:
  explicit ManifestLock(const std::filesystem::path& manifest)
      : path(manifest.string() + ".zucchini-update.lock") {
    if (!std::filesystem::create_directory(path)) {
      throw std::runtime_error("another manifest update is in progress; retry discovery");
    }
  }
  ~ManifestLock() {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
  }
private:
  std::filesystem::path path;
};
std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot read '" + path.string() + "'");
  }
  std::ostringstream contents;
  contents << file.rdbuf();
  if (file.bad()) {
    throw std::runtime_error("cannot read '" + path.string() + "'");
  }
  return contents.str();
}
void validate(const std::string& text) {
  ParsedManifest manifest;
  Diagnostics errors;
  if (!parse_step_def_manifest(text, manifest, errors)) {
    throw std::runtime_error("invalid step definitions:" + to_string(errors));
  }
}
void collect_steps(const Json& steps, std::set<std::string>& patterns,
                   std::set<std::string>& methods) {
  for (const auto& step : steps) {
    if (step.contains("group")) {
      collect_steps(step.at("group").at("steps"), patterns, methods);
    } else {
      patterns.insert(step.at("step").get<std::string>());
      methods.insert(step.at("methodName").get<std::string>());
    }
  }
}
std::string unique_name(std::string base, std::set<std::string>& names) {
  auto result = base;
  std::size_t suffix = 2;
  while (!names.insert(result).second) {
    result = base + std::to_string(suffix++);
  }
  return result;
}
nlohmann::ordered_json ordered(const Json& value) {
  if (value.is_array()) {
    auto result = nlohmann::ordered_json::array();
    for (const auto& item : value) result.push_back(ordered(item));
    return result;
  }
  if (!value.is_object()) return value;
  auto result = nlohmann::ordered_json::object();
  for (const auto* key : {"step", "methodName", "arguments", "dataTable",
                          "name", "kind", "fields", "type", "content", "header", "optional"}) {
    if (value.contains(key)) result[key] = ordered(value.at(key));
  }
  for (const auto& item : value.items()) {
    if (!result.contains(item.key())) result[item.key()] = ordered(item.value());
  }
  return result;
}
void style_new_node(fy_node* node, const Json& value, bool pattern = false) {
  if (value.is_object()) {
    fy_node_set_style(node, FYNS_BLOCK);
    void* iterator = nullptr;
    while (auto* pair = fy_node_mapping_iterate(node, &iterator)) {
      auto* key = fy_node_pair_key(pair);
      const std::string name = fy_node_get_scalar0(key);
      fy_node_set_style(key, FYNS_PLAIN);
      style_new_node(fy_node_pair_value(pair), value.at(name), name == "step");
    }
  } else if (value.is_array()) {
    fy_node_set_style(node, FYNS_BLOCK);
    for (std::size_t index = 0; index < value.size(); ++index) {
      style_new_node(fy_node_sequence_get_by_index(node, static_cast<int>(index)), value[index]);
    }
  } else if (value.is_string()) {
    if (pattern) {
      fy_node_set_style(node, FYNS_SINGLE_QUOTED);
    } else {
      // Keep strings quoted when their plain spelling would change their value.
      try {
        if (YamlDocument(value.get<std::string>()).json() == value) {
          fy_node_set_style(node, FYNS_PLAIN);
        }
      } catch (const std::exception&) {}
    }
  }
}
std::string emit_node(fy_node* node, std::size_t indent,
                      const std::string& newline, bool original = false) {
  const auto flags = static_cast<fy_emitter_cfg_flags>(
      (original ? FYECF_MODE_BLOCK : FYECF_MODE_MANUAL) |
      FYECF_INDENT_2 | FYECF_WIDTH_INF | FYECF_STRIP_DOC | FYECF_OUTPUT_COMMENTS);
  std::unique_ptr<char, decltype(&std::free)> output(
      fy_emit_node_to_string(node, flags), &std::free);
  if (!output) throw std::runtime_error("cannot emit new YAML definitions");
  std::istringstream lines(output.get());
  std::string line;
  std::string result;
  while (std::getline(lines, line)) {
    result += std::string(indent, ' ') + line + newline;
  }
  return result;
}
std::string scalar(const Json& value, bool pattern = false) {
  YamlDocument document(value.dump());
  style_new_node(document.root(), value, pattern);
  auto text = emit_node(document.root(), 0, "\n");
  if (!text.empty() && text.back() == '\n') text.pop_back();
  return text;
}
std::string emit(const Json& value, std::size_t indent,
                 const std::string& newline) {
  const std::string padding(indent, ' ');
  std::string result;
  if (value.is_array()) {
    for (const auto& item : value) {
      if (!result.empty()) result += newline;
      auto text = emit(item, indent + 2, newline);
      text.replace(0, indent + 2, padding + "- ");
      result += text;
    }
  } else if (value.is_object()) {
    const auto fields = ordered(value);
    for (const auto& item : fields.items()) {
      result += padding + scalar(item.key()) + ":";
      const Json child = item.value();
      if ((child.is_array() || child.is_object()) && !child.empty()) {
        result += newline + emit(child, indent + 2, newline);
      } else {
        result += " " + scalar(child, item.key() == "step") + newline;
      }
    }
  } else {
    result = padding + scalar(value) + newline;
  }
  return result;
}
std::size_t position(fy_token* token, bool end = false) {
  const auto* mark = end ? fy_token_end_mark(token) : fy_token_start_mark(token);
  if (!mark) {
    throw std::runtime_error("YAML node has no source position");
  }
  return mark->input_pos;
}
std::size_t line_start(const std::string& text, std::size_t offset) {
  const auto preceding = offset ? text.rfind('\n', offset - 1) : std::string::npos;
  return preceding == std::string::npos ? 0 : preceding + 1;
}
// Expand collection punctuation using library source marks. Existing entries
// and their comments remain verbatim; only newly proposed entries are emitted.
std::string expand_flow(fy_node* node, const std::string& source,
                         std::size_t indent, const std::string& newline) {
  std::vector<std::pair<std::size_t, std::size_t>> entries;
  void* iterator = nullptr;
  if (fy_node_is_mapping(node)) {
    while (auto* pair = fy_node_mapping_iterate(node, &iterator)) {
      entries.emplace_back(position(fy_node_get_start_token(fy_node_pair_key(pair))),
                           position(fy_node_get_end_token(fy_node_pair_value(pair)), true));
    }
  } else {
    while (auto* item = fy_node_sequence_iterate(node, &iterator)) {
      entries.emplace_back(position(fy_node_get_start_token(item)),
                           position(fy_node_get_end_token(item), true));
    }
  }
  std::string result;
  const auto close = position(fy_node_get_end_token(node));
  const auto opening = position(fy_node_get_start_token(node), true);
  const auto prefix = source.substr(opening, (entries.empty() ? close : entries.front().first) - opening);
  if (prefix.find('#') != std::string::npos) result += prefix;
  for (std::size_t index = 0; index < entries.size(); ++index) {
    const auto [start, end] = entries[index];
    result += std::string(indent, ' ') + (fy_node_is_sequence(node) ? "- " : "") +
              source.substr(start, end - start);
    const auto next = index + 1 < entries.size() ? entries[index + 1].first : close;
    auto gap = source.substr(end, next - end);
    const auto comma = gap.find_first_not_of(" \t\r\n");
    if (comma != std::string::npos && gap[comma] == ',') gap.erase(comma, 1);
    if (gap.find('#') != std::string::npos) {
      const auto last = gap.find_last_not_of(" \t\r\n");
      result += gap.substr(0, last + 1);
    }
    result += newline;
  }
  return result;
}
void append_section(fy_node* root, const std::string& source,
                    const std::string& key, const Json& additions,
                    std::vector<Edit>& edits) {
  if (additions.empty()) {
    return;
  }
  const std::string newline = source.find("\r\n") != std::string::npos ? "\r\n" : "\n";
  auto* section = fy_node_mapping_lookup_value_by_simple_key(root, key.data(), key.size());
  if (!section) {
    const auto offset = position(fy_node_get_end_token(root));
    const auto atLineStart = offset == line_start(source, offset);
    edits.push_back({offset, (atLineStart ? "" : newline) + key + ":" + newline +
                            emit(additions, 2, newline)});
    return;
  }
  if (!fy_node_is_sequence(section)) {
    throw std::runtime_error("cannot insert into aliased or non-sequence '" + key + "'");
  }
  const auto offset = position(fy_node_get_end_token(section));
  if (fy_node_get_style(section) == FYNS_FLOW) {
    auto start = position(fy_node_get_start_token(section));
    if (start && source[start - 1] == ' ') --start;
    const auto end = position(fy_node_get_end_token(section), true);
    auto* keyNode = fy_node_mapping_lookup_pair_by_simple_key(root, key.data(), key.size());
    const auto* mark = fy_token_start_mark(fy_node_get_start_token(fy_node_pair_key(keyNode)));
    const auto indent = static_cast<std::size_t>(mark->column) + 2;
    // Remove only the flow container, retaining the section's inline comment.
    auto insertion = source.find('\n', end);
    const bool hasNewline = insertion != std::string::npos;
    insertion = hasNewline ? insertion + 1 : source.size();
    std::string existing;
    if (fy_node_sequence_item_count(section)) {
      existing = expand_flow(section, source, indent, newline) + newline;
    }
    edits.push_back({start, "", end - start});
    edits.push_back({insertion, (hasNewline ? "" : newline) + existing + emit(additions, indent, newline)});
  } else {
    const auto* start = fy_token_start_mark(fy_node_get_start_token(section));
    if (!start) {
      throw std::runtime_error("YAML sequence has no indentation information");
    }
    const auto atLineStart = offset == line_start(source, offset);
    edits.push_back({offset, (atLineStart ? "" : newline) + newline +
                            emit(additions, static_cast<std::size_t>(start->column), newline)});
  }
}
}

void update_manifest(const std::string& path, const std::string& suggestionsPath) {
  const auto target = std::filesystem::canonical(path);
  const ManifestLock lock(target);
  const auto source = read_file(target);
  validate(source);
  const auto current = YamlDocument(source).json();
  const auto suggestedText = read_file(suggestionsPath);
  validate(suggestedText);
  const auto suggestions = YamlDocument(suggestedText).json();
  auto steps = Json::array();
  auto types = Json::array();
  std::set<std::string> patterns;
  std::set<std::string> methods{"around_step", "validate_scenario"};
  std::set<std::string> typeNames;
  collect_steps(current.at("steps"), patterns, methods);
  for (const auto& type : current.value("types", Json::array())) {
    typeNames.insert(type.at("name").get<std::string>());
  }
  for (auto step : suggestions.at("steps")) {
    if (!patterns.insert(step.at("step").get<std::string>()).second) {
      continue;
    }
    step["methodName"] = unique_name(step.at("methodName").get<std::string>(), methods);
    if (step.contains("dataTable")) {
      const auto name = step.at("dataTable").at("type").get<std::string>();
      const auto& proposedTypes = suggestions.at("types");
      const auto found = std::find_if(proposedTypes.begin(), proposedTypes.end(),
          [&](const Json& type) { return type.at("name") == name; });
      if (found == proposedTypes.end()) {
        throw std::runtime_error("missing suggested table type '" + name + "'");
      }
      auto type = *found;
      const auto newName = unique_name(name, typeNames);
      type["name"] = newName;
      step["dataTable"]["type"] = newName;
      types.push_back(std::move(type));
    }
    steps.push_back(std::move(step));
  }
  if (steps.empty()) {
    return; // A stale executable must not insert the same definitions twice.
  }

  auto editable = source;
  YamlDocument shape(source, false);
  if (fy_node_get_style(shape.root()) == FYNS_FLOW) {
    const auto start = position(fy_node_get_start_token(shape.root()));
    const auto end = position(fy_node_get_end_token(shape.root()), true);
    const std::string newline = source.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    editable.replace(start, end - start, expand_flow(shape.root(), source, 0, newline));
  }
  YamlDocument original(editable, false);
  std::vector<Edit> edits;
  append_section(original.root(), editable, "types", types, edits);
  append_section(original.root(), editable, "steps", steps, edits);
  std::stable_sort(edits.begin(), edits.end(),
      [](const Edit& lhs, const Edit& rhs) { return lhs.position > rhs.position; });
  auto candidate = editable;
  for (const auto& edit : edits) {
    candidate.replace(edit.position, edit.length, edit.text);
  }
  validate(candidate);

  const auto temporary = target.string() + ".zucchini-" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tmp";
  try {
    {
      std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
      file << candidate;
      file.close();
      if (!file) {
        throw std::runtime_error("cannot write updated step definitions");
      }
    }
    std::filesystem::permissions(temporary, std::filesystem::status(target).permissions());
    if (read_file(target) != source) {
      throw std::runtime_error("manifest changed during update; retry discovery");
    }
#ifdef _WIN32
    if (!MoveFileExW(std::filesystem::path(temporary).c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      throw std::runtime_error("cannot replace updated manifest");
    }
#else
    std::filesystem::rename(temporary, target);
#endif
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    throw;
  }
}
}
