#pragma once

namespace nZucchini {
// How narrowly a business requirement pins down the outcomes it accepts. The
// rule author declares it; reports display it without judging coverage by it.
enum class Strictness {
  Unspecified,
  // The check names the one expected outcome.
  Exact,
  // The check lists the acceptable outcomes.
  OneOf,
  // The check limits the outcome on every side, in any number of dimensions.
  Bounded,
  // The check rules some outcomes out but leaves the rest open.
  OpenEnded
};

inline const char *strictness_name(Strictness strictness) {
  switch (strictness) {
  case Strictness::Exact:
    return "exact";
  case Strictness::OneOf:
    return "one_of";
  case Strictness::Bounded:
    return "bounded";
  case Strictness::OpenEnded:
    return "open_ended";
  case Strictness::Unspecified:
    break;
  }
  return "unspecified";
}
} // namespace nZucchini
