#include "Zucchini/Discovery.hpp"

#include "Zucchini/FeatureParser.hpp"
#include "Zucchini/ManifestStore.hpp"
#include "Zucchini/Naming.hpp"
#include "Zucchini/Snippets.hpp"

#include <stdexcept>
#include <utility>

namespace nZucchini
{
    namespace
    {
        std::function<std::vector<Zucchini>()> &provider()
        {
            static std::function<std::vector<Zucchini>()> instance;
            return instance;
        }

        void report(const std::string &what, const Diagnostics &diagnostics)
        {
            if (!diagnostics.empty())
            {
                std::cerr << what << to_string(diagnostics) << std::endl;
            }
        }

        [[noreturn]] void fail()
        {
            exit(EXIT_FAILURE);
        }

        std::optional<std::string> option_value(const std::string &argument, const std::string &key)
        {
            const auto prefix = key + '=';
            if (argument.rfind(prefix, 0) == 0)
            {
                return argument.substr(prefix.size());
            }
            return std::nullopt;
        }

        class LazyZucchiniGenerator : public testing::internal::ParamGeneratorInterface<Zucchini>
        {
        public:
            using Base = testing::internal::ParamGeneratorInterface<Zucchini>;
            using Iterator = testing::internal::ParamIteratorInterface<Zucchini>;

            Iterator *Begin() const override
            {
                load();
                return new Cursor(this, 0);
            }

            Iterator *End() const override
            {
                load();
                return new Cursor(this, values.size());
            }

        private:
            void load() const
            {
                if (loaded)
                {
                    return;
                }
                if (!provider())
                {
                    throw std::runtime_error(
                        "no zucchini provider installed; call nZucchini::install_zucchini_provider");
                }
                values = provider()();
                loaded = true;
            }

            class Cursor : public Iterator
            {
            public:
                Cursor(const LazyZucchiniGenerator *owner, std::size_t index)
                    : owner(owner), index(index)
                {
                }

                const Base *BaseGenerator() const override
                {
                    return owner;
                }

                void Advance() override
                {
                    ++index;
                }

                Iterator *Clone() const override
                {
                    return new Cursor(*this);
                }

                const Zucchini *Current() const override
                {
                    return &owner->values[index];
                }

                bool Equals(const Iterator &other) const override
                {
                    return index == static_cast<const Cursor &>(other).index;
                }

            private:
                const LazyZucchiniGenerator *owner;
                std::size_t index;
            };

            mutable std::vector<Zucchini> values;
            mutable bool loaded = false;
        };
    }

    DiscoveryArgs parse_discovery_args(int argc, char **argv)
    {
        DiscoveryArgs args;
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];
            if (const auto featureDir = option_value(argument, "feature_dir"))
            {
                args.featureDir = *featureDir;
            }
            else if (const auto manifestDir = option_value(argument, "manifest_dir"))
            {
                args.manifestDir = *manifestDir;
            }
        }
        return args;
    }

    std::vector<Zucchini> discover_zucchinis(const DiscoveryArgs &args,
                                             const StepDefManifest &manifest,
                                             const ScenarioValidator &validate)
    {
        FeatureParseResult parsed;
        Diagnostics errors;
        const auto parsedOk = parse_feature_dir(args.featureDir, manifest, parsed, errors);

        if (!parsed.undefinedSteps.empty())
        {
            throw std::runtime_error("undefined steps in '" + args.featureDir + "'; add these step definitions:\n\n" + step_snippets(parsed.undefinedSteps));
        }

        if (!errors.empty())
        {
            report("cannot parse features in '" + args.featureDir + "':", errors);
            const auto fatal = has_errors(errors);
            errors.clear();
            if (fatal)
            {
                fail();
            }
        }
        if (!parsedOk)
        {
            return {};
        }

        std::vector<Zucchini> zucchinis;
        Diagnostics scenarioDiagnostics;
        for (const auto &scenario : parsed.scenarios)
        {
            if (validate)
            {
                validate(scenario.zucchini, scenario.pickle, scenarioDiagnostics);
            }
            zucchinis.push_back(scenario.zucchini);
        }

        if (!scenarioDiagnostics.empty())
        {
            report("scenario diagnostics:", scenarioDiagnostics);
            if (has_errors(scenarioDiagnostics))
            {
                fail();
            }
        }

        if (!args.manifestDir.empty() && !store_zucchinis(args.manifestDir, zucchinis, errors))
        {
            report("cannot store zucchini manifests in '" + args.manifestDir + "':", errors);
            if (has_errors(errors))
            {
                fail();
            }
        }

        return zucchinis;
    }

    std::vector<Zucchini> load_discovered_zucchinis(const DiscoveryArgs &args)
    {
        std::vector<Zucchini> zucchinis;
        Diagnostics errors;
        if (!load_zucchinis(args.manifestDir, zucchinis, errors))
        {
            report("cannot load zucchini manifests from '" + args.manifestDir + "':", errors);
            if (has_errors(errors))
            {
                fail();
            }
        }
        return zucchinis;
    }

    void install_zucchini_provider(int argc, char **argv, StepDefManifest manifest, ScenarioValidator validate)
    {
        set_zucchini_provider([args = parse_discovery_args(argc, argv),
                               manifest = std::move(manifest),
                               validate = std::move(validate)]
                              { return args.featureDir.empty() ? load_discovered_zucchinis(args)
                                                               : discover_zucchinis(args, manifest, validate); });
    }

    void set_zucchini_provider(std::function<std::vector<Zucchini>()> zucchiniProvider)
    {
        provider() = std::move(zucchiniProvider);
    }

    testing::internal::ParamGenerator<Zucchini> zucchini_values()
    {
        return testing::internal::ParamGenerator<Zucchini>(new LazyZucchiniGenerator());
    }

    std::string zucchini_test_name(const testing::TestParamInfo<Zucchini> &info)
    {
        return test_name(info.param);
    }
}
