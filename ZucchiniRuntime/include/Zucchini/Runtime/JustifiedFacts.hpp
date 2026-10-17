#pragma once

#include <cstddef>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace nZucchini {
// The asserted facts business rules claimed as coverage in one scenario: whole
// steps, or single data table cells keyed like DataTableArgument rows.
class JustifiedFacts {
public:
  void justify(std::size_t step) { steps.insert(step); }
  void justify(std::size_t step, std::size_t row, std::string key) {
    cells.emplace(step, row, std::move(key));
  }

  bool justifies(std::size_t step) const { return steps.count(step) != 0; }
  bool justifies(std::size_t step, std::size_t row,
                 const std::string &key) const {
    return cells.count(std::make_tuple(step, row, key)) != 0;
  }

private:
  std::set<std::size_t> steps;
  std::set<std::tuple<std::size_t, std::size_t, std::string>> cells;
};
} // namespace nZucchini
