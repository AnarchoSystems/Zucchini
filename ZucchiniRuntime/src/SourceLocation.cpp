#include "Zucchini/Runtime/SourceLocation.hpp"

namespace nZucchini {
namespace {
thread_local SourceLocation currentLocation;
thread_local bool hasCurrentLocation = false;
} // namespace

void set_current_source_location(SourceLocation location) {
  currentLocation = std::move(location);
  hasCurrentLocation = true;
}

void clear_current_source_location() {
  currentLocation = SourceLocation();
  hasCurrentLocation = false;
}

const SourceLocation *current_source_location() {
  return hasCurrentLocation ? &currentLocation : nullptr;
}

std::string to_string(const SourceLocation &location) {
  return location.uri + ':' + std::to_string(location.line) + ':' +
         std::to_string(location.column);
}

void print_current_source_location(std::ostream &stream) {
  const auto *location = current_source_location();
  if (location == nullptr) {
    return;
  }
  stream << "gherkin source:\n" << to_string(*location);
  if (!location->stepText.empty()) {
    stream << ": " << location->stepText;
  }
  stream << std::endl;
}
} // namespace nZucchini
