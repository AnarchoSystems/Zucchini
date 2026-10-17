#pragma once

#include "Zucchini/Runtime/Zucchini.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nZucchini {
// One cell of a step's data table, named by its record index and either its
// header (keyed tables) or its position (headerless tables). Records are rows,
// or columns for column-oriented tables, in the order rules receive them.
// Generated fixtures provide table_cell(row, &Struct::member) for typed rows.
class TableCell {
public:
  TableCell(std::size_t row, std::string header)
      : row(row), headers{std::move(header)} {}
  TableCell(std::size_t row, std::size_t position)
      : row(row), position(position) {}
  // A struct field: any one of its alternative headers, or its position.
  TableCell(std::size_t row, std::vector<std::string> headers,
            std::size_t position)
      : row(row), headers(std::move(headers)), position(position) {}

  // The cell's key in the step's DataTableArgument row, or nothing when the
  // table does not contain it. Throws when the step has no such record.
  std::optional<std::string> key(const ZucchiniStep &step) const {
    const auto *table = std::get_if<DataTableArgument>(&step.argument);
    if (table == nullptr || row >= table->rows.size()) {
      throw std::out_of_range("step '" + step.text + "' has no data table row " +
                              std::to_string(row));
    }
    const auto &record = table->rows[row];
    if (record.is_object()) {
      for (const auto &header : headers) {
        if (record.contains(header)) {
          return header;
        }
      }
      return std::nullopt;
    }
    if (position && *position < record.size()) {
      return std::to_string(*position);
    }
    return std::nullopt;
  }

  std::size_t row;

private:
  std::vector<std::string> headers;
  std::optional<std::size_t> position;
};
} // namespace nZucchini
