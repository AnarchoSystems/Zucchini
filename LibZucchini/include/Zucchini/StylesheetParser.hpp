#pragma once

#include "Zucchini/Diagnostics.hpp"
#include "Zucchini/Stylesheet.hpp"

#include <string>

namespace nZucchini {
bool parse_stylesheet(const std::string &yaml, Stylesheet &stylesheet,
                      Diagnostics &errors);
bool parse_stylesheet_from_file(const std::string &filePath,
                                Stylesheet &stylesheet, Diagnostics &errors);
} // namespace nZucchini
