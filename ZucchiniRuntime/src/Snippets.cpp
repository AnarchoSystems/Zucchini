#include "Snippets.hpp"

#include "Naming.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace nZucchini {
namespace {
struct SnippetCapture {
  std::string name;
  std::string type;
};

bool is_digit(char character) {
  return std::isdigit(static_cast<unsigned char>(character)) != 0;
}

void escape_regex(char character, std::string &out) {
  static const std::string special = R"(\^$.|?*+()[]{})";
  if (special.find(character) != std::string::npos) {
    out.push_back('\\');
  }
  out.push_back(character);
}

void append_name(const std::string &text, std::string &name, bool &capitalize) {
  for (const auto character : text) {
    if (std::isalpha(static_cast<unsigned char>(character)) == 0) {
      capitalize = !name.empty();
      continue;
    }
    if (capitalize) {
      name.push_back(static_cast<char>(std::toupper(character)));
    } else {
      name.push_back(name.empty() ? static_cast<char>(std::tolower(character))
                                  : character);
    }
    capitalize = false;
  }
}

// Collects the raw words of an identifier (space-separated, original case);
// casing is applied later via apply_casing so the same words can be rendered
// snake_case, camelCase or CamelCase.
void append_word_text(const std::string &text, std::string &name,
                      bool &pendingSeparator) {
  for (const auto character : text) {
    if (std::isalpha(static_cast<unsigned char>(character)) == 0) {
      if (!name.empty()) {
        pendingSeparator = true;
      }
      continue;
    }
    if (pendingSeparator) {
      name.push_back(' ');
    }
    name.push_back(character);
    pendingSeparator = false;
  }
}

// Numbers and quoted strings become capture groups; everything else is matched
// literally.
std::string regex_for(const std::string &stepText,
                      std::vector<SnippetCapture> &captures,
                      std::string &name) {
  std::string pattern = "^";
  bool pendingSeparator = false;
  for (std::size_t index = 0; index < stepText.size();) {
    if (stepText[index] == '"') {
      const auto closing = stepText.find('"', index + 1);
      if (closing != std::string::npos) {
        captures.push_back(
            {"arg" + std::to_string(captures.size() + 1), "string"});
        pattern += R"RX("([^"]*)")RX";
        index = closing + 1;
        if (!name.empty()) {
          pendingSeparator = true;
        }
        continue;
      }
    }

    const auto negative = stepText[index] == '-' &&
                          index + 1 < stepText.size() &&
                          is_digit(stepText[index + 1]);
    if (negative || is_digit(stepText[index])) {
      std::size_t cursor = index + (negative ? 1 : 0);
      while (cursor < stepText.size() && is_digit(stepText[cursor])) {
        ++cursor;
      }
      auto isDecimal = false;
      if (cursor + 1 < stepText.size() && stepText[cursor] == '.' &&
          is_digit(stepText[cursor + 1])) {
        isDecimal = true;
        ++cursor;
        while (cursor < stepText.size() && is_digit(stepText[cursor])) {
          ++cursor;
        }
      }

      captures.push_back({"arg" + std::to_string(captures.size() + 1),
                          isDecimal ? "float" : "int"});
      pattern += isDecimal ? R"((-?\d+\.\d+))" : R"((-?\d+))";
      index = cursor;
      if (!name.empty()) {
        pendingSeparator = true;
      }
      continue;
    }

    escape_regex(stepText[index], pattern);
    append_word_text(std::string(1, stepText[index]), name, pendingSeparator);
    ++index;
  }
  return pattern + '$';
}

std::string method_name_of(const std::string &stepText, NameCasing casing) {
  std::vector<SnippetCapture> captures;
  std::string name;
  regex_for(stepText, captures, name);
  return apply_casing(name.empty() ? "step" : name, casing);
}

std::string pattern_of(const UndefinedStep &step) {
  std::vector<SnippetCapture> captures;
  std::string name;
  return regex_for(step.text, captures, name);
}

// Missing columns become optional; types must fit every observed value.
void merge_table(
    std::optional<std::vector<UndefinedTableColumn>> &target,
    const std::optional<std::vector<UndefinedTableColumn>> &incoming) {
  if (!incoming) {
    return;
  }
  if (!target) {
    target = incoming;
    return;
  }

  for (auto &column : *target) {
    const auto found = std::find_if(
        incoming->begin(), incoming->end(),
        [&](const UndefinedTableColumn &other) {
          return other.header == column.header;
        });
    if (found == incoming->end()) {
      column.optional = true;
      continue;
    }
    column.optional = column.optional || found->optional;
    column.couldBeInt = column.couldBeInt && found->couldBeInt;
    column.couldBeDouble = column.couldBeDouble && found->couldBeDouble;
    column.couldBeBool = column.couldBeBool && found->couldBeBool;
  }
  for (const auto &incomingColumn : *incoming) {
    const auto found = std::find_if(
        target->begin(), target->end(),
        [&](const UndefinedTableColumn &other) {
          return other.header == incomingColumn.header;
        });
    if (found == target->end()) {
      auto column = incomingColumn;
      column.optional = true;
      target->push_back(std::move(column));
    }
  }
}

// A header sanitized the same way step text is: words become a camelCase
// identifier.
std::string identifier_from(const std::string &header, NameCasing casing) {
  std::string name;
  bool capitalize = false;
  append_name(header, name, capitalize);
  return apply_casing(name, casing);
}

std::string table_type_name(const std::string &stepText, NameCasing casing) {
  return method_name_of(stepText, casing) + "Row";
}

std::string yaml_quote(const std::string &text) {
  std::string quoted = "\"";
  for (const auto character : text) {
    if (character == '"' || character == '\\') {
      quoted.push_back('\\');
    }
    quoted.push_back(character);
  }
  quoted.push_back('"');
  return quoted;
}

std::string yaml_single_quote(const std::string &text) {
  std::string quoted = "'";
  for (const auto character : text) {
    quoted.push_back(character);
    if (character == '\'') {
      quoted.push_back('\'');
    }
  }
  quoted.push_back('\'');
  return quoted;
}

// Every column starts out able to be int/double/bool; each disconfirming
// example rules one out. Prefer the narrowest type that still fits every
// observed value; string is the fallback.
std::string column_type(const UndefinedTableColumn &column) {
  if (column.couldBeInt) {
    return "int";
  }
  if (column.couldBeDouble) {
    return "float";
  }
  if (column.couldBeBool) {
    return "bool";
  }
  return {};
}

std::string
table_type_snippet(const std::string &typeName,
                   const std::vector<UndefinedTableColumn> &columns,
                   NameCasing variablesCasing) {
  std::ostringstream snippet;
  snippet << "  - name: " << typeName << '\n';
  snippet << "    kind: struct\n";
  snippet << "    fields:\n";
  for (const auto &column : columns) {
    auto fieldName = identifier_from(column.header, variablesCasing);
    if (fieldName.empty()) {
      fieldName = "field";
    }
    snippet << "      - name: " << fieldName << '\n';
    if (const auto type = column_type(column); !type.empty()) {
      snippet << "        type: " << type << '\n';
    }
    if (fieldName != column.header) {
      snippet << "        header: " << yaml_quote(column.header) << '\n';
    }
    if (column.optional) {
      snippet << "        optional: true\n";
    }
  }
  return snippet.str();
}
} // namespace

void merge_undefined_step(std::vector<UndefinedStep> &steps,
                          UndefinedStep incoming) {
  const auto pattern = pattern_of(incoming);
  const auto existing = std::find_if(
      steps.begin(), steps.end(), [&](const UndefinedStep &step) {
        return pattern_of(step) == pattern;
      });
  if (existing == steps.end()) {
    steps.push_back(std::move(incoming));
    return;
  }
  merge_table(existing->table, incoming.table);
}

std::string step_snippet(const UndefinedStep &step, NameCasing methodsCasing,
                         NameCasing classesCasing,
                         NameCasing variablesCasing) {
  std::vector<SnippetCapture> captures;
  std::string name;
  const auto pattern = regex_for(step.text, captures, name);
  const auto methodName =
      apply_casing(name.empty() ? "step" : name, methodsCasing);

  std::ostringstream snippet;
  snippet << "  - step: " << yaml_single_quote(pattern) << '\n';
  snippet << "    methodName: " << methodName << '\n';

  if (!captures.empty()) {
    snippet << "    arguments:\n";
    for (const auto &capture : captures) {
      snippet << "      - name: " << capture.name << '\n';
      snippet << "        type: " << capture.type << '\n';
    }
  }

  if (step.table) {
    snippet << "    dataTable:\n";
    snippet << "      type: " << table_type_name(step.text, classesCasing)
            << '\n';
  }

  return snippet.str();
}

std::string step_snippets(const std::vector<UndefinedStep> &steps,
                          NameCasing methodsCasing,
                          NameCasing classesCasing,
                          NameCasing variablesCasing) {
  if (steps.empty()) {
    return {};
  }

  std::vector<UndefinedStep> uniqueSteps;
  for (const auto &step : steps) {
    merge_undefined_step(uniqueSteps, step);
  }

  std::ostringstream snippet;

  const auto hasTables = std::any_of(
      uniqueSteps.begin(), uniqueSteps.end(), [](const UndefinedStep &step) {
        return step.table.has_value();
      });
  if (hasTables) {
    snippet << "types:\n";
    for (const auto &step : uniqueSteps) {
      if (step.table) {
        snippet << table_type_snippet(table_type_name(step.text, classesCasing),
                                      *step.table, variablesCasing);
      }
    }
    snippet << '\n';
  }

  snippet << "steps:\n";
  for (const auto &step : uniqueSteps) {
    snippet << step_snippet(step, methodsCasing, classesCasing, variablesCasing);
  }
  return snippet.str();
}
} // namespace nZucchini
