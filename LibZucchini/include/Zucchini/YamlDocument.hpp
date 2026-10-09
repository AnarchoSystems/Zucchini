#pragma once
#include <nlohmann/json.hpp>
#include <string>

struct fy_document;
struct fy_node;
struct fy_diag;

namespace nZucchini {
// Owns the input bytes and libfyaml's document and diagnostic objects.
class YamlDocument {
public:
  explicit YamlDocument(std::string source, bool resolve = true);
  ~YamlDocument();
  YamlDocument(const YamlDocument&) = delete;
  YamlDocument& operator=(const YamlDocument&) = delete;
  fy_node* root() const;
  nlohmann::json json() const;
private:
  std::string source;
  fy_diag* diag = nullptr;
  fy_document* document = nullptr;
};

bool yaml_to_json(const std::string& yaml, nlohmann::json& json,
                  std::string& error);
}
