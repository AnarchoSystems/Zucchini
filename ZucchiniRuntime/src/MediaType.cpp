#include "Zucchini/Runtime/MediaType.hpp"

#include "YamlToJson.hpp"

#include <stdexcept>

namespace nZucchini {
nlohmann::json
MediaTypeConverter<json>::convertToJSON(const std::string &content) {
  return nlohmann::json::parse(content);
}

nlohmann::json
MediaTypeConverter<yaml>::convertToJSON(const std::string &content) {
  nlohmann::json json;
  std::string error;
  if (!yaml_to_json(content, json, error)) {
    throw std::runtime_error("cannot read YAML content: " + error);
  }
  return json;
}
} // namespace nZucchini
