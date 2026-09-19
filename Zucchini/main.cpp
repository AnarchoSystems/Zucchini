#include "Lowering.hpp"

#include <Zucchini/Diagnostics.hpp>
#include <Zucchini/ManifestParser.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ZucchiniTemplates_functions.h"

namespace
{
    struct Options
    {
        std::string yamlPath;
        std::string fixture;
        std::string outputDir = ".";
    };

    void usage()
    {
        std::cerr << "usage: Zucchini -i <yaml> -fixture <fixture name> [-o <output dir>]\n";
    }

    bool parse_options(int argc, char** argv, Options& options)
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];
            const auto value = [&]() -> std::string {
                return index + 1 < argc ? argv[++index] : std::string();
            };

            if (argument == "-i")
            {
                options.yamlPath = value();
            }
            else if (argument == "-fixture")
            {
                options.fixture = value();
            }
            else if (argument == "-o")
            {
                options.outputDir = value();
            }
            else
            {
                std::cerr << "Zucchini: unknown option '" << argument << "'\n";
                return false;
            }
        }

        return !options.yamlPath.empty() && !options.fixture.empty();
    }

    bool write_file(const std::filesystem::path& path, const std::string& contents)
    {
        std::ofstream file(path);
        if (!file)
        {
            std::cerr << "Zucchini: cannot write '" << path.string() << "'\n";
            return false;
        }
        file << contents;
        return true;
    }
}

int main(int argc, char** argv)
{
    Options options;
    if (!parse_options(argc, argv, options))
    {
        usage();
        return 2;
    }

    std::ifstream yamlFile(options.yamlPath);
    if (!yamlFile)
    {
        std::cerr << "Zucchini: cannot read '" << options.yamlPath << "'\n";
        return 1;
    }

    std::ostringstream yamlContents;
    yamlContents << yamlFile.rdbuf();
    const auto yaml = yamlContents.str();

    nZucchini::StepDefManifest manifest;
    nZucchini::Diagnostics errors;
    const auto manifestOk = nZucchini::parse_step_def_manifest(yaml, manifest, errors);
    if (!errors.empty())
    {
        std::cerr << options.yamlPath << ": error: invalid step definitions:"
                  << nZucchini::to_string(errors) << '\n';
    }
    if (!manifestOk && nZucchini::has_errors(errors))
    {
        return 1;
    }

    try
    {
        const auto fixture = nZucchini::lower(manifest, options.fixture, yaml);
        const std::filesystem::path directory = options.outputDir;
        std::filesystem::create_directories(directory);

        if (!write_file(directory / ("I" + options.fixture + ".h"),
                        nZucchiniTemplates::render_header(fixture)))
        {
            return 1;
        }
        if (!write_file(directory / (options.fixture + "Test.cc"),
                        nZucchiniTemplates::render_test(fixture)))
        {
            return 1;
        }
    }
    catch (const std::exception& failure)
    {
        std::cerr << options.yamlPath << ": error: " << failure.what() << '\n';
        return 1;
    }

    return 0;
}
