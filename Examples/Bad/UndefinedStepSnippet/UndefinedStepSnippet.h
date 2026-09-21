#pragma once

#include "IUndefinedStepSnippet.h"

namespace nUndefinedStepSnippet {
// No step is defined yet; the manifest below is deliberately empty so discovery
// fails and prints a ready-to-paste snippet for the step used by the feature.
class UndefinedStepSnippet : public IUndefinedStepSnippet {};
} // namespace nUndefinedStepSnippet
