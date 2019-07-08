#pragma once

#include <fstream>
#include "glm/glm.hpp"
#include <vector>

namespace serialize {

    template <typename T>
    inline void write(std::ofstream& out, const T& val) {
        out.write((char*) &val, sizeof(T));
    }

    template <>
    inline void write(std::ofstream& out, const std::string& s) {
        uint32_t len = s.length();
        out.write((char*) &len, sizeof(uint32_t));
        out.write(s.c_str(), len);
    }

    template <>
    inline void write(std::ofstream& out, const glm::vec3& v) {
        out.write((char*) &v.x, sizeof(glm::vec3));
    }

    template <typename T>
    inline void write(std::ofstream& out, const std::vector<T>& vec) {
        size_t len = vec.size();
        serialize::write(out, len);
        if (len)
            out.write((char*) &vec[0], len * sizeof(T));
    }

    inline void write(std::ofstream& out, char* buff, size_t len) {
        out.write(buff, len);
    }

    template <typename T>
    inline void read(std::ifstream& in, T& val) {
        in.read((char*) &val, sizeof(T));
    }

    template <>
    inline void read(std::ifstream& in, std::string& s) {
        uint32_t len;
        in.read((char*) &len, sizeof(uint32_t));
        s.resize(len);
        in.read((char*) &s[0], len);
    }

    template <>
    inline void read(std::ifstream& in, glm::vec3& v) {
        in.read((char*) &v.x, sizeof(glm::vec3));
    }

    template <typename T>
    inline void read(std::ifstream& in, std::vector<T>& vec) {
        size_t len;
        serialize::read(in, len);
        vec.resize(len);
        if (len)
            in.read((char*) &vec[0], len * sizeof(T));
    }

    inline void read(std::ifstream& in, char* buff, size_t len) {
        in.read(buff, len);
    }

} // namespace serializeUtils