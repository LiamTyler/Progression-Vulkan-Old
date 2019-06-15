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

    bool loadShaderFromText(Shader& shader, const ShaderFileDesc& desc);

    bool loadShaderFromBinary(Shader& shader, const char* binarySource, GLint len, GLenum format);

    char* getShaderBinary(const Shader& shader, GLint& len, GLenum& format);

} // namespace Progression
