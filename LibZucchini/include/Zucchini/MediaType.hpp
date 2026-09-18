#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace nZucchini
{
    // Phantom media types, named exactly like the content type in a DocString or a markdown fence.
    // Declare your own tag plus a MediaTypeConverter specialisation and a manifest that names that
    // contentType picks it up through the generated code.
    struct json
    {
    };

    struct yaml
    {
    };

    template <typename MediaType>
    struct MediaTypeConverter;

    template <>
    struct MediaTypeConverter<json>
    {
        static nlohmann::json convertToJSON(const std::string& content);
    };

    template <>
    struct MediaTypeConverter<yaml>
    {
        static nlohmann::json convertToJSON(const std::string& content);
    };
}
