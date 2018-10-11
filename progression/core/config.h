#pragma once

#define YAML_CPP_DLL
#include "yaml-cpp/yaml.h"

namespace Progression { namespace config {

    class Config {
    public:
        Config();
        Config(const std::string& fname);
        ~Config() = default;

        explicit operator bool() const;

        template <typename T>
        T as() const {
            T ret;
            try {
                ret = node.as<T>();
            }
            catch (std::exception e) {
                std::cout << "Error converting: " << e.what() << std::endl;
            }
            return ret;
        }

        template <typename T>
        Config operator[](const T& key) const {
            return Config(node[key]);
        }

        template <typename T>
        void set(const std::string& key, T value) {
            node[key] = value;
        }

        friend Config LoadFile(const std::string& fname);

    private:
        Config(const YAML::Node& n);

        YAML::Node node;
        bool valid;
    };

} } // namespace Progression::config