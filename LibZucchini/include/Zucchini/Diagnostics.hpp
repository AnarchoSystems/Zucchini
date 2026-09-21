#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace nZucchini {
enum class DiagnosticSeverity { Error, Warning, Info };

using CodingPath = std::string;

inline std::string coding_key(std::string key) { return key; }

inline std::string coding_index(std::size_t index) {
  return std::string("[") + std::to_string(index) + "]";
}

struct Diagnostic {
  Diagnostic() = default;
  Diagnostic(std::string path, std::string message,
             std::optional<std::uint32_t> line = std::nullopt,
             std::optional<std::uint32_t> column = std::nullopt,
             DiagnosticSeverity severity = DiagnosticSeverity::Error)
      : path(std::move(path)), message(std::move(message)), line(line),
        column(column), severity(severity) {}

  std::string path;
  std::string message;
  std::optional<std::uint32_t> line;
  std::optional<std::uint32_t> column;
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
};

using Diagnostics = std::vector<Diagnostic>;

bool operator==(const Diagnostic &lhs, const Diagnostic &rhs);
std::string to_string(const Diagnostic &diagnostic);
std::ostream &operator<<(std::ostream &stream, const Diagnostic &diagnostic);

std::string to_string(const Diagnostics &diagnostics);
std::vector<std::string> paths_of(const Diagnostics &diagnostics);
bool has_errors(const Diagnostics &diagnostics);

void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message);
void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message, DiagnosticSeverity severity);
void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message, std::uint32_t line,
                    std::uint32_t column,
                    DiagnosticSeverity severity = DiagnosticSeverity::Error);
} // namespace nZucchini
