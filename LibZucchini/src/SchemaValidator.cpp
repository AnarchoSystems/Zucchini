#include "SchemaValidator.hpp"

#include "SchemaSource.hpp"

#include <nlohmann/json-schema.hpp>

#include <cctype>

namespace nZucchini
{
    namespace
    {
        CodingPath path_of(const std::string& pointer)
        {
            CodingPath path;
            std::size_t index = 0;
            while (index < pointer.size())
            {
                const auto separator = pointer.find('/', index + 1);
                auto token = pointer.substr(index + 1, separator - index - 1);
                index = separator == std::string::npos ? pointer.size() : separator;

                const auto numeric = !token.empty()
                    && token.find_first_not_of("0123456789") == std::string::npos;
                if (numeric)
                {
                    path.push_back(coding_index(static_cast<std::size_t>(std::stoul(token))));
                }
                else
                {
                    path.push_back(coding_key(std::move(token)));
                }
            }
            return path;
        }

        class DiagnosticHandler : public nlohmann::json_schema::basic_error_handler
        {
        public:
            explicit DiagnosticHandler(Diagnostics& errors)
                : errors(errors)
            {
            }

            void error(const nlohmann::json::json_pointer& pointer,
                       const nlohmann::json& instance,
                       const std::string& message) override
            {
                nlohmann::json_schema::basic_error_handler::error(pointer, instance, message);
                add_diagnostic(errors, path_of(pointer.to_string()), message);
            }

        private:
            Diagnostics& errors;
        };

        const nlohmann::json_schema::json_validator& validator()
        {
            static const auto instance = [] {
                nlohmann::json_schema::json_validator created;
                created.set_root_schema(nlohmann::json::parse(kStepDefSchema));
                return created;
            }();
            return instance;
        }
    }

    bool validate_against_schema(const nlohmann::json& document, Diagnostics& errors)
    {
        DiagnosticHandler handler(errors);
        validator().validate(document, handler);
        return !handler;
    }
}
