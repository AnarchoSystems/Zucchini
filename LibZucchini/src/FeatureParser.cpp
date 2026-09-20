#include "Zucchini/FeatureParser.hpp"

#include "Zucchini/MakeZucchini.hpp"
#include "Zucchini/Naming.hpp"

#include <cucumber/gherkin/app.hpp>
#include <cucumber/messages/background.hpp>
#include <cucumber/messages/feature.hpp>
#include <cucumber/messages/feature_child.hpp>
#include <cucumber/messages/gherkin_document.hpp>
#include <cucumber/messages/rule.hpp>
#include <cucumber/messages/rule_child.hpp>
#include <cucumber/messages/scenario.hpp>
#include <cucumber/messages/step.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace nZucchini
{
    namespace
    {
        namespace messages = cucumber::messages;

        struct ScenarioNames
        {
            std::string feature;
            std::string rule;
        };

        struct AstIndex
        {
            std::map<std::string, ScenarioNames> scenarios;
            std::map<std::string, messages::location> steps;
        };

        void index_scenario(const messages::scenario& scenario,
                            const std::string& featureName,
                            const std::string& ruleName,
                            AstIndex& index)
        {
            index.scenarios.emplace(scenario.id, ScenarioNames{featureName, ruleName});
            for (const auto& step : scenario.steps)
            {
                index.steps.emplace(step.id, step.location);
            }
        }

        void index_background(const messages::background& background, AstIndex& index)
        {
            for (const auto& step : background.steps)
            {
                index.steps.emplace(step.id, step.location);
            }
        }

        AstIndex index_document(const messages::gherkin_document& document)
        {
            AstIndex index;
            if (!document.feature)
            {
                return index;
            }

            const auto& feature = *document.feature;
            for (const auto& child : feature.children)
            {
                if (child.background)
                {
                    index_background(*child.background, index);
                }
                if (child.scenario)
                {
                    index_scenario(*child.scenario, feature.name, {}, index);
                }
                if (child.rule)
                {
                    for (const auto& ruleChild : child.rule->children)
                    {
                        if (ruleChild.background)
                        {
                            index_background(*ruleChild.background, index);
                        }
                        if (ruleChild.scenario)
                        {
                            index_scenario(*ruleChild.scenario, feature.name, child.rule->name, index);
                        }
                    }
                }
            }

            return index;
        }

        void apply_locations(const AstIndex& index, const messages::pickle& pickle, Zucchini& zucchini)
        {
            for (std::size_t step = 0; step < zucchini.steps.size() && step < pickle.steps.size(); ++step)
            {
                for (const auto& nodeId : pickle.steps[step].ast_node_ids)
                {
                    const auto location = index.steps.find(nodeId);
                    if (location == index.steps.end())
                    {
                        continue;
                    }
                    zucchini.steps[step].line = static_cast<std::uint32_t>(location->second.line);
                    zucchini.steps[step].column =
                        static_cast<std::uint32_t>(location->second.column.value_or(0));
                    break;
                }
            }
        }

        const messages::location* step_location(const AstIndex& index,
                                                const messages::pickle& pickle,
                                                std::size_t stepIndex)
        {
            if (stepIndex >= pickle.steps.size())
            {
                return nullptr;
            }
            for (const auto& nodeId : pickle.steps[stepIndex].ast_node_ids)
            {
                const auto location = index.steps.find(nodeId);
                if (location != index.steps.end())
                {
                    return &location->second;
                }
            }
            return nullptr;
        }

        bool split_step_path(const CodingPath& path, std::size_t& stepIndex, std::string& detail)
        {
            constexpr char prefix[] = "steps[";
            if (path.rfind(prefix, 0) != 0)
            {
                return false;
            }
            const auto close = path.find(']', sizeof(prefix) - 1);
            if (close == std::string::npos)
            {
                return false;
            }
            try
            {
                stepIndex = static_cast<std::size_t>(std::stoull(path.substr(sizeof(prefix) - 1,
                                                                            close - (sizeof(prefix) - 1))));
            }
            catch (const std::exception&)
            {
                return false;
            }
            detail = close + 1 < path.size() && path[close + 1] == '.' ? path.substr(close + 2) : std::string();
            return true;
        }

        // Scenario outline rows and independently named scenarios can sanitize to the same name.
        void deduplicate(std::vector<Scenario>& scenarios)
        {
            std::set<std::string> seen;
            for (auto& scenario : scenarios)
            {
                const auto originalName = scenario.zucchini.name;
                std::size_t suffix = 1;
                while (!seen.insert(test_name(scenario.zucchini)).second)
                {
                    scenario.zucchini.name = originalName + " #" + std::to_string(++suffix);
                }
            }
        }

        void collect_undefined(const messages::pickle& pickle,
                               const StepDefManifest& manifest,
                               std::vector<std::string>& undefined)
        {
            for (const auto& step : pickle.steps)
            {
                if (find_step_def(manifest, step.text) != nullptr)
                {
                    continue;
                }
                if (std::find(undefined.begin(), undefined.end(), step.text) == undefined.end())
                {
                    undefined.push_back(step.text);
                }
            }
        }
    }

    bool parse_feature(const std::string& source,
                       const std::string& uri,
                       const StepDefManifest& manifest,
                       FeatureParseResult& result,
                       Diagnostics& errors)
    {
        result = FeatureParseResult();
        errors.clear();

        messages::source document;
        document.uri = uri;
        document.data = source;
        document.media_type = messages::source_media_type::TEXT_X_CUCUMBER_GHERKIN_PLAIN;

        cucumber::gherkin::app parser;
        parser.include_source(false);
        parser.include_ast(true);
        parser.include_pickles(true);

        messages::gherkin_document ast;
        std::vector<messages::pickle> pickles;

        cucumber::gherkin::app::callbacks callbacks;
        callbacks.ast = [&ast](const messages::gherkin_document& parsed) { ast = parsed; };
        callbacks.pickle = [&pickles](const messages::pickle& pickle) { pickles.push_back(pickle); };
        callbacks.error = [&errors, &uri](const cucumber::gherkin::parse_error& failure) {
            add_diagnostic(errors,
                           uri,
                           failure.message,
                           static_cast<std::uint32_t>(failure.location.line),
                           static_cast<std::uint32_t>(failure.location.column.value_or(0)));
        };

        parser.parse(document, callbacks);

        if (!errors.empty())
        {
            return false;
        }

        const auto index = index_document(ast);

        for (std::size_t pickle = 0; pickle < pickles.size(); ++pickle)
        {
            collect_undefined(pickles[pickle], manifest, result.undefinedSteps);

            ScenarioNames names;
            for (const auto& nodeId : pickles[pickle].ast_node_ids)
            {
                const auto scenario = index.scenarios.find(nodeId);
                if (scenario != index.scenarios.end())
                {
                    names = scenario->second;
                    break;
                }
            }

            Zucchini zucchini;
            Diagnostics stepErrors;
            if (!make_zucchini(pickles[pickle], manifest, names.feature, zucchini, stepErrors))
            {
                for (auto& error : stepErrors)
                {
                    std::size_t stepIndex = 0;
                    std::string detail;
                    if (split_step_path(error.path, stepIndex, detail))
                    {
                        if (const auto* location = step_location(index, pickles[pickle], stepIndex))
                        {
                            error.path = uri;
                            error.line = static_cast<std::uint32_t>(location->line);
                            error.column = static_cast<std::uint32_t>(location->column.value_or(0));
                            if (!detail.empty())
                            {
                                error.message = detail + ": " + error.message;
                            }
                            errors.push_back(std::move(error));
                            continue;
                        }
                    }

                    std::string prefixed = uri + "[" + std::to_string(pickle) + "]";
                    if (!error.path.empty())
                    {
                        prefixed += "." + error.path;
                    }
                    error.path = std::move(prefixed);
                    errors.push_back(std::move(error));
                }
                continue;
            }

            zucchini.ruleName = names.rule;
            zucchini.uri = uri;
            apply_locations(index, pickles[pickle], zucchini);
            result.scenarios.push_back(Scenario{std::move(zucchini), pickles[pickle]});
        }

        if (!errors.empty())
        {
            result.scenarios.clear();
            return false;
        }

        deduplicate(result.scenarios);
        return true;
    }

    bool parse_feature_file(const std::string& path,
                            const StepDefManifest& manifest,
                            FeatureParseResult& result,
                            Diagnostics& errors)
    {
        result = FeatureParseResult();
        errors.clear();

        std::ifstream file(path);
        if (!file)
        {
            add_diagnostic(errors, path, "cannot open feature file");
            return false;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return parse_feature(contents.str(), path, manifest, result, errors);
    }

    bool parse_feature_dir(const std::string& directory,
                           const StepDefManifest& manifest,
                           FeatureParseResult& result,
                           Diagnostics& errors)
    {
        result = FeatureParseResult();
        errors.clear();

        std::error_code failure;
        if (!std::filesystem::is_directory(directory, failure))
        {
            add_diagnostic(errors, directory, "not a feature directory");
            return false;
        }

        std::vector<std::string> paths;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, failure))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".feature")
            {
                paths.push_back(entry.path().string());
            }
        }
        std::sort(paths.begin(), paths.end());

        for (const auto& path : paths)
        {
            FeatureParseResult parsed;
            Diagnostics fileErrors;
            const auto ok = parse_feature_file(path, manifest, parsed, fileErrors);

            for (auto& stepText : parsed.undefinedSteps)
            {
                if (std::find(result.undefinedSteps.begin(), result.undefinedSteps.end(), stepText)
                    == result.undefinedSteps.end())
                {
                    result.undefinedSteps.push_back(std::move(stepText));
                }
            }

            if (!ok)
            {
                errors.insert(errors.end(), fileErrors.begin(), fileErrors.end());
                continue;
            }

            result.scenarios.insert(result.scenarios.end(),
                                    std::make_move_iterator(parsed.scenarios.begin()),
                                    std::make_move_iterator(parsed.scenarios.end()));
        }

        if (!errors.empty())
        {
            result.scenarios.clear();
            return false;
        }

        deduplicate(result.scenarios);
        return true;
    }
}
