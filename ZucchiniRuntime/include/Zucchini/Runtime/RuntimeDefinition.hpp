#pragma once

#include <optional>
#include <string>
#include <vector>

namespace nZucchini {
enum class CaptureConversion { Text, Integer, Number, Boolean, Ignore };
enum class TableOrientation { Rows, Columns };
enum class NameCasing { SnakeCase, CamelCase, PascalCase };

struct CaptureDefinition {
  std::string name;
  CaptureConversion conversion = CaptureConversion::Text;
};

struct DataTableDefinition {
  TableOrientation orientation = TableOrientation::Rows;
  bool header = true;
};

struct DocStringDefinition {
  std::optional<std::string> mediaType;
};

struct StepDefinition {
  std::string regex;
  std::string methodName;
  std::vector<CaptureDefinition> captures;
  std::optional<DataTableDefinition> dataTable;
  std::optional<DocStringDefinition> docString;
};

struct StepDefinitions {
  std::vector<StepDefinition> steps;
};
} // namespace nZucchini