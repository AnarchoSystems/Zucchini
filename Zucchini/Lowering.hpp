#pragma once

#include <Zucchini/StepDefManifest.hpp>

#include <string>

#include "ZucchiniTemplates_types.h"

namespace nZucchini
{
    // Lowers a step definition manifest into the render model used by the tpp templates.
    nZucchiniTemplates::Fixture lower(const StepDefManifest& manifest,
                                      const std::string& fixtureName,
                                      const std::string& yaml);
}
