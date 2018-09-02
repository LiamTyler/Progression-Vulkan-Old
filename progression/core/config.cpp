#include "core/config.h"
#include <filesystem>
#include <iostream>

namespace Progression { namespace config {

    Config LoadFile(const std::string& name) {
        Config ret;

        std::filesystem::path file(name);
        if (file.extension() != ".yaml") {
            std::cout << "Config needs a .yaml file as input" << std::endl;
            ret.valid = false;
        } else if (!std::filesystem::exists(file)) {
            std::cout << "File: " << name << " does not exist" << std::endl;
            ret.valid = false;
        } else {
            try {
                ret.node = YAML::LoadFile(name);
            }
            catch (std::exception e) {
                std::cout << "Error while parsing: " << e.what() << std::endl;
                ret.valid = false;
            }
        }
        return ret;
    }

    Config::Config() :
        node(),
        valid(true)
    {
    }

    Config::Config(const std::string& fname) {
        *this = LoadFile(fname);
    }

    Config::operator bool() const {
        return node && valid;
    }

    Config::Config(const YAML::Node& n) :
        node(n),
        valid(true)
    {
    }
} } // namespace Progression::config
