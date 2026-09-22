#pragma once
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>

// Stands in for a legacy string class that predates std::string in some
// codebases: it owns a heap-allocated C string rather than an std::string, and
// exposes just equality and streaming.
namespace nLegacyString {
class LegacyString {
public:
  LegacyString() : LegacyString("") {}
  LegacyString(const char *value) : value_(copy(value)) {}
  LegacyString(const LegacyString &other) : value_(copy(other.value_.get())) {}

  LegacyString &operator=(const LegacyString &other) {
    value_ = copy(other.value_.get());
    return *this;
  }

  friend bool operator==(const LegacyString &lhs, const LegacyString &rhs) {
    return std::strcmp(lhs.value_.get(), rhs.value_.get()) == 0;
  }

  friend bool operator<(const LegacyString &lhs, const LegacyString &rhs) {
    return std::strcmp(lhs.value_.get(), rhs.value_.get()) < 0;
  }

  friend std::ostream &operator<<(std::ostream &stream,
                                  const LegacyString &value) {
    return stream << value.value_.get();
  }

  const char *c_str() const { return value_.get(); }

private:
  static std::unique_ptr<char[]> copy(const char *value) {
    const auto length = std::strlen(value);
    auto buffer = std::make_unique<char[]>(length + 1);
    std::memcpy(buffer.get(), value, length + 1);
    return buffer;
  }

  std::unique_ptr<char[]> value_;
};

inline void to_json(nlohmann::json &json, const LegacyString &value) {
  json = value.c_str();
}

inline void from_json(const nlohmann::json &json, LegacyString &value) {
  value = LegacyString(json.get<std::string>().c_str());
}
} // namespace nLegacyString
