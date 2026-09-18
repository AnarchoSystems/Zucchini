#include "YamlToJson.hpp"

#include <fkYAML/node.hpp>

namespace nZucchini
{
    namespace
    {
        nlohmann::json convert(const fkyaml::node& node)
        {
            if (node.is_boolean())
            {
                return node.get_value<bool>();
            }
            if (node.is_integer())
            {
                return node.get_value<std::int64_t>();
            }
            if (node.is_float_number())
            {
                return node.get_value<double>();
            }
            if (node.is_string())
            {
                return node.get_value<std::string>();
            }
            if (node.is_sequence())
            {
                auto array = nlohmann::json::array();
                for (const auto& element : node.as_seq())
                {
                    array.push_back(convert(element));
                }
                return array;
            }
            if (node.is_mapping())
            {
                auto object = nlohmann::json::object();
                for (const auto& entry : node.as_map())
                {
                    object[entry.first.is_string() ? entry.first.get_value<std::string>()
                                                   : fkyaml::node::serialize(entry.first)] =
                        convert(entry.second);
                }
                return object;
            }
            return nlohmann::json();
        }
    }

    bool yaml_to_json(const std::string& yaml, nlohmann::json& json, std::string& error)
    {
        try
        {
            json = convert(fkyaml::node::deserialize(yaml));
        }
        catch (const fkyaml::exception& failure)
        {
            error = failure.what();
            return false;
        }
        return true;
    }
}
