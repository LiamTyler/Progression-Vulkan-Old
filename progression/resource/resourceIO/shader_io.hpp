#pragma once

#include "resource/shader.hpp"
#include <string>

namespace Progression {

    typedef struct ShaderFilesDesc {
        std::string vertex;
        std::string geometry;
        std::string fragment;
        std::string compute;
    } ShaderFileDesc;

    void addShaderRootDir(ShaderFileDesc& desc, const std::string& root);

    bool loadShaderFromText(Shader& shader, const ShaderFileDesc& desc);

    bool loadShaderFromBinary(Shader& shader, const char* binarySource, GLint len, GLenum format);

    char* getShaderBinary(const Shader& shader, GLint& len, GLenum& format);

    bool getShaderInfoFromResourceFile(std::string& name, ShaderFileDesc& desc, std::istream& in);

} // namespace Progression
