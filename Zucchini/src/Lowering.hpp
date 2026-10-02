#pragma once

#include "ManifestParser.hpp"

#include "Stylesheet.hpp"

#include <string>

#include "ZucchiniTemplates_types.h"

namespace nZucchini {
// Lowers a step definition manifest into the render model used by the tpp
// templates.
nZucchiniTemplates::Fixture lower(const StepDefinitions &manifest,
                                  const Stylesheet &stylesheet,
                                  const std::string &fixtureName);
nZucchiniTemplates::Fixture lower(const ParsedManifest &manifest,
                                  const Stylesheet &stylesheet,
                                  const std::string &fixtureName);
} // namespace nZucchini
