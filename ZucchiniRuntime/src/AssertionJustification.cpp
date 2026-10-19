#include "Zucchini/Runtime/AssertionJustification.hpp"

namespace nZucchini {
void to_json(nlohmann::json &json, const AssertionJustification &summary) {
  // "enforced" is deprecated but required until the next major schema version.
  json = {{"enforced", false},
          {"scenarios", summary.scenarios},
          {"scenarios_without_assertions", summary.scenariosWithoutAssertions},
          {"asserted_facts", summary.assertedFacts},
          {"unjustified_facts", summary.unjustifiedFacts}};
}
} // namespace nZucchini
