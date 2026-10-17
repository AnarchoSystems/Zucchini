#pragma once

#include "Zucchini/Runtime/SourceLocation.hpp"

#include <cucumber/messages/pickle.hpp>

#include <string>
#include <vector>

namespace nZucchini {
// Feature-file positions of a data table's cells, by source row and column.
using TableCellLocations = std::vector<std::vector<SourceLocation>>;

struct PickleScenario {
  cucumber::messages::pickle pickle;
  std::string featureName;
  std::string ruleName;
  std::string uri;
  std::uint32_t line = 0;
  std::vector<SourceLocation> stepLocations;
  // Parallel to stepLocations; empty for steps without a data table.
  std::vector<TableCellLocations> stepTableCells;
};
} // namespace nZucchini