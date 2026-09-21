#include "Zucchini/Diagnostics.hpp"

#include <algorithm>
#include <sstream>

namespace nZucchini {
bool operator==(const Diagnostic &lhs, const Diagnostic &rhs) {
  return lhs.path == rhs.path && lhs.message == rhs.message &&
         lhs.line == rhs.line && lhs.column == rhs.column &&
         lhs.severity == rhs.severity;
}

std::string to_string(const Diagnostic &diagnostic) {
  std::ostringstream stream;
  stream << diagnostic;
  return stream.str();
}

std::ostream &operator<<(std::ostream &stream, const Diagnostic &diagnostic) {
  const auto location =
      diagnostic.path.empty() ? std::string("<document>") : diagnostic.path;
  const auto severity =
      diagnostic.severity == DiagnosticSeverity::Error     ? "error"
      : diagnostic.severity == DiagnosticSeverity::Warning ? "warning"
                                                           : "info";
  if (diagnostic.line) {
    stream << location << ':' << *diagnostic.line << ':'
           << (diagnostic.column.value_or(0)) << ": " << severity << ": ";
  } else {
    stream << location << ": " << severity << ": ";
  }
  return stream << diagnostic.message;
}

std::string to_string(const Diagnostics &diagnostics) {
  std::ostringstream stream;
  for (const auto &diagnostic : diagnostics) {
    stream << "\n" << diagnostic;
  }
  return stream.str();
}

std::vector<std::string> paths_of(const Diagnostics &diagnostics) {
  std::vector<std::string> paths;
  paths.reserve(diagnostics.size());
  for (const auto &diagnostic : diagnostics) {
    paths.push_back(diagnostic.path);
  }
  return paths;
}

bool has_errors(const Diagnostics &diagnostics) {
  return std::any_of(diagnostics.begin(), diagnostics.end(),
                     [](const auto &diagnostic) {
                       return diagnostic.severity == DiagnosticSeverity::Error;
                     });
}

void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message) {
  add_diagnostic(diagnostics, std::move(path), std::move(message),
                 DiagnosticSeverity::Error);
}

void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message, DiagnosticSeverity severity) {
  diagnostics.push_back(Diagnostic{std::move(path), std::move(message),
                                   std::nullopt, std::nullopt, severity});
}

void add_diagnostic(Diagnostics &diagnostics, std::string path,
                    std::string message, std::uint32_t line,
                    std::uint32_t column, DiagnosticSeverity severity) {
  diagnostics.push_back(
      Diagnostic{std::move(path), std::move(message), line, column, severity});
}
} // namespace nZucchini
