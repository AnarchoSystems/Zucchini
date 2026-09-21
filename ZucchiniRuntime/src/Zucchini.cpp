#include "Zucchini/Runtime/Zucchini.hpp"

#include "Naming.hpp"

namespace nZucchini {
bool operator==(const Capture &lhs, const Capture &rhs) {
  return lhs.name == rhs.name && lhs.value == rhs.value;
}

bool operator==(const DocStringArgument &lhs, const DocStringArgument &rhs) {
  return lhs.content == rhs.content && lhs.mediaType == rhs.mediaType;
}

bool operator==(const DataTableArgument &lhs, const DataTableArgument &rhs) {
  return lhs.rows == rhs.rows;
}

bool operator==(const ZucchiniStep &lhs, const ZucchiniStep &rhs) {
  return lhs.regex == rhs.regex && lhs.methodName == rhs.methodName &&
         lhs.text == rhs.text && lhs.captures == rhs.captures &&
         lhs.argument == rhs.argument && lhs.line == rhs.line &&
         lhs.column == rhs.column;
}

bool operator==(const Zucchini &lhs, const Zucchini &rhs) {
  return lhs.name == rhs.name && lhs.featureName == rhs.featureName &&
         lhs.ruleName == rhs.ruleName && lhs.uri == rhs.uri &&
         lhs.steps == rhs.steps && lhs.tags == rhs.tags;
}

void to_json(nlohmann::json &json, const Capture &capture) {
  json = nlohmann::json{{"name", capture.name}, {"value", capture.value}};
}

void from_json(const nlohmann::json &json, Capture &capture) {
  json.at("name").get_to(capture.name);
  capture.value = json.at("value");
}

void to_json(nlohmann::json &json, const DocStringArgument &docString) {
  json = nlohmann::json{{"kind", "docString"}, {"content", docString.content}};
  if (docString.mediaType) {
    json["mediaType"] = *docString.mediaType;
  }
}

void from_json(const nlohmann::json &json, DocStringArgument &docString) {
  json.at("content").get_to(docString.content);
  docString.mediaType.reset();
  if (const auto mediaType = json.find("mediaType");
      mediaType != json.end() && !mediaType->is_null()) {
    docString.mediaType = mediaType->get<std::string>();
  }
}

void to_json(nlohmann::json &json, const DataTableArgument &dataTable) {
  json = nlohmann::json{{"kind", "dataTable"}, {"rows", dataTable.rows}};
}

void from_json(const nlohmann::json &json, DataTableArgument &dataTable) {
  json.at("rows").get_to(dataTable.rows);
}

void to_json(nlohmann::json &json, const ZucchiniStep &step) {
  json = nlohmann::json{
      {"regex", step.regex},         {"methodName", step.methodName},
      {"text", step.text},           {"line", step.line},
      {"column", step.column},       {"captures", step.captures},
      {"argument", nlohmann::json()}};

  std::visit(
      [&json](const auto &argument) {
        using ArgumentType = std::decay_t<decltype(argument)>;
        if constexpr (!std::is_same_v<ArgumentType, std::monostate>) {
          json["argument"] = argument;
        }
      },
      step.argument);
}

void from_json(const nlohmann::json &json, ZucchiniStep &step) {
  json.at("regex").get_to(step.regex);
  json.at("methodName").get_to(step.methodName);
  json.at("text").get_to(step.text);
  json.at("line").get_to(step.line);
  json.at("column").get_to(step.column);
  json.at("captures").get_to(step.captures);

  step.argument = std::monostate{};
  const auto argument = json.find("argument");
  if (argument == json.end() || argument->is_null()) {
    return;
  }

  const auto kind = argument->at("kind").get<std::string>();
  if (kind == "docString") {
    step.argument = argument->get<DocStringArgument>();
  } else if (kind == "dataTable") {
    step.argument = argument->get<DataTableArgument>();
  } else {
    throw nlohmann::json::other_error::create(
        501, "unknown step argument kind: " + kind, &*argument);
  }
}

void to_json(nlohmann::json &json, const Zucchini &zucchini) {
  json = nlohmann::json{{"name", zucchini.name},
                        {"feature", zucchini.featureName},
                        {"rule", zucchini.ruleName},
                        {"uri", zucchini.uri},
                        {"steps", zucchini.steps}};
  if (!zucchini.tags.empty()) {
    json["tags"] = zucchini.tags;
  }
}

void from_json(const nlohmann::json &json, Zucchini &zucchini) {
  json.at("name").get_to(zucchini.name);
  json.at("feature").get_to(zucchini.featureName);
  json.at("rule").get_to(zucchini.ruleName);
  json.at("uri").get_to(zucchini.uri);
  zucchini.tags.clear();
  if (const auto tags = json.find("tags");
      tags != json.end() && !tags->is_null()) {
    tags->get_to(zucchini.tags);
  }
  json.at("steps").get_to(zucchini.steps);
}

std::ostream &operator<<(std::ostream &stream, const ZucchiniStep &step) {
  return stream << nlohmann::json(step).dump(2);
}

std::ostream &operator<<(std::ostream &stream, const Zucchini &zucchini) {
  return stream << display_name(zucchini);
}
} // namespace nZucchini
