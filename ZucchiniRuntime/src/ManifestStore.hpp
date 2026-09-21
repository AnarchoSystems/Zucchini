#pragma once

#include <Zucchini/Diagnostics.hpp>

#include "Zucchini/Runtime/Zucchini.hpp"

#include <string>
#include <vector>

namespace nZucchini {
// Zucchinis are stored as one JSON file per test name, so a running test can
// look its own up.
bool store_zucchini(const std::string &directory, const Zucchini &zucchini,
                    Diagnostics &errors);
bool store_zucchinis(const std::string &directory,
                     const std::vector<Zucchini> &zucchinis,
                     Diagnostics &errors);

bool load_zucchini(const std::string &directory, const std::string &testName,
                   Zucchini &zucchini, Diagnostics &errors);
bool load_zucchinis(const std::string &directory,
                    std::vector<Zucchini> &zucchinis, Diagnostics &errors);

std::string manifest_path(const std::string &directory,
                          const std::string &testName);
} // namespace nZucchini
