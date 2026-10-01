#pragma once

#include "Zucchini/Runtime/Zucchini.hpp"

#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nZucchini {
using DataTableRow = std::map<std::string, std::string>;
using PositionalDataTableRow = std::vector<std::string>;

class DataTable {
public:
  static DataTable from_step(const ZucchiniStep &step) {
    const auto *argument = std::get_if<DataTableArgument>(&step.argument);
    if (argument == nullptr) {
      throw std::runtime_error("step '" + step.text + "' has no data table");
    }

    DataTable table;
    for (const auto &row : argument->rows) {
      DataTableRow keyedRow;
      if (row.is_object()) {
        for (const auto &cell : row.items()) {
          keyedRow[cell.key()] = cell_text(cell.value());
        }
      } else {
        for (std::size_t column = 0; column < row.size(); ++column) {
          keyedRow[std::to_string(column)] = cell_text(row[column]);
        }
      }
      table.rows.push_back(std::move(keyedRow));

      PositionalDataTableRow positionalRow;
      for (const auto &cell : row) {
        positionalRow.push_back(cell_text(cell));
      }
      table.positionalRows.push_back(std::move(positionalRow));
    }
    return table;
  }

  template <typename String = std::string>
  std::vector<std::map<String, String>> dictionary_rows() const {
    std::vector<std::map<String, String>> result;
    result.reserve(rows.size());
    for (const auto &row : rows) {
      std::map<String, String> converted;
      for (const auto &cell : row) {
        converted.emplace(String(cell.first.c_str()), String(cell.second.c_str()));
      }
      result.push_back(std::move(converted));
    }
    return result;
  }

  template <typename String = std::string>
  std::vector<std::vector<String>> positional_rows() const {
    std::vector<std::vector<String>> result;
    result.reserve(positionalRows.size());
    for (const auto &row : positionalRows) {
      std::vector<String> converted;
      converted.reserve(row.size());
      for (const auto &cell : row) {
        converted.emplace_back(cell.c_str());
      }
      result.push_back(std::move(converted));
    }
    return result;
  }

  std::vector<DataTableRow> rows;
  std::vector<PositionalDataTableRow> positionalRows;

private:
  static std::string cell_text(const nlohmann::json &cell) {
    return cell.is_string() ? cell.get<std::string>() : cell.dump();
  }
};

inline const DocStringArgument &doc_string(const ZucchiniStep &step) {
  const auto *content = std::get_if<DocStringArgument>(&step.argument);
  if (content == nullptr) {
    throw std::runtime_error("step '" + step.text + "' has no DocString");
  }
  return *content;
}

inline std::string require_cell(const DataTableRow &row,
                                const std::vector<std::string> &headers) {
  for (const auto &header : headers) {
    const auto cell = row.find(header);
    if (cell != row.end()) {
      return cell->second;
    }
  }

  std::string message =
      "data table is missing a required column; expected one of: ";
  for (std::size_t index = 0; index < headers.size(); ++index) {
    message += (index == 0 ? "'" : ", '") + headers[index] + "'";
  }
  throw std::runtime_error(message);
}

inline std::string cell_or(const DataTableRow &row,
                           const std::vector<std::string> &headers,
                           const std::string &fallback) {
  for (const auto &header : headers) {
    const auto cell = row.find(header);
    if (cell != row.end()) {
      return cell->second;
    }
  }
  return fallback;
}

inline std::vector<std::string> split_cell(const std::string &value,
                                           char separator) {
  std::vector<std::string> parts;
  std::istringstream stream(value);
  std::string part;
  while (std::getline(stream, part, separator)) {
    const auto first = part.find_first_not_of(" \t\r\n");
    const auto last = part.find_last_not_of(" \t\r\n");
    part = first == std::string::npos
               ? std::string()
               : part.substr(first, last - first + 1);
    parts.push_back(part);
  }
  return parts;
}

inline long to_long(const std::string &value) { return std::stol(value); }

inline double to_double(const std::string &value) { return std::stod(value); }

inline bool to_bool(const std::string &value) {
  return value == "true" || value == "True" || value == "TRUE" ||
         value == "yes" || value == "Yes" || value == "YES" || value == "1";
}
} // namespace nZucchini