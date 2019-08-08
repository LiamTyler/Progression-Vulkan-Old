#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "core/common.hpp"

namespace fileIO {

    template <typename T>
    void parse(std::istream& in, T& val) {
        in >> val;
        PG_ASSERT(!in.fail());
    }

    template <typename T>
    void parseLineKeyVal(std::istream& in, const char* key, T& val) {
        std::string line;
        std::string s;
        std::getline(in, line);
        auto ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == key);
        ss >> val;
        PG_ASSERT(!in.fail() && !ss.fail());
    }

    template <typename T>
    bool parseLineKeyValOptional(std::istream& in, const char* key, T& val) {
        std::string line;
        std::string s;
        std::getline(in, line);
        auto ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == key);
        if (!ss.eof())
            ss >> val;
        else
            return false;
        
        PG_ASSERT(!in.fail() && !ss.fail());
        return true;
    }

    inline bool parseLineKeyBool(std::istream& in, const char* key) {
        std::string line;
        std::string s;
        std::getline(in, line);
        auto ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == key);
        ss >> s;
        PG_ASSERT(!in.fail() && !ss.fail());
        PG_ASSERT(s == "true" || s == "false");
        return s == "true" ? true : false;
    }

    template <typename T>
    bool parseLineKeyMap(std::istream& in, const char* key, const std::unordered_map<std::string, T>& map, T& val) {
        std::string line;
        std::string s;
        std::getline(in, line);
        auto ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == key);
        ss >> s;
        PG_ASSERT(!in.fail() && !ss.fail());
        auto it = map.find(s);
        if (it == map.end())
            return false;

        val = it->second;
        return true;
    }

} // namespace fileIO