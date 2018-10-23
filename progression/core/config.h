#pragma once

#include "cpptoml.h"

namespace Progression { namespace config {
	
    class Config {
    public:
		Config(const std::string& fname);
		Config(std::shared_ptr<cpptoml::table>& handle);
		~Config() = default;

		explicit operator bool() const;

		std::shared_ptr<cpptoml::table> operator->() const {
			return handle_;
		}
		/*
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
		*/

        /*template <typename T>
        Config operator[](const T& key) const {
            return Config(node[key]);
        }*/

    private:
		std::shared_ptr<cpptoml::table> handle_;
    };

	Config parseFile(const std::string& path);

} } // namespace Progression::config