#include "resource/resourceIO/shader_io.hpp"
#include "utils/logger.hpp"
#include <fstream>

namespace Progression {

    namespace {

        // TODO: faster way
        bool loadShaderTextFile(std::string& source, const std::string& filename) {
            std::ifstream in(filename);
            if (in.fail()) {
                LOG_ERR("Failed to open the shader file:", filename);
                return false; 
            }
            source = "";
            std::string line;
            while (std::getline(in, line))
                source += line + '\n';
            in.close();
            return true;
        }

        bool compileShader(GLuint& shader, const char* source, GLenum shaderType) {
            shader = glCreateShader(shaderType);
            glShaderSource(shader, 1, &source, NULL);
            glCompileShader(shader);

            GLint result = GL_FALSE;
            int infoLogLength;

            glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            if (!result) {
                std::vector<char> errorMessage(infoLogLength + 1);
                glGetShaderInfoLog(shader, infoLogLength, NULL, &errorMessage[0]);
                std::string err(&errorMessage[0]);
                LOG_ERR("Error while loading shader:\n", err, '\n');
                glDeleteShader(shader);
                return false;
            }

            return true;
        }

        bool createAndLinkProgram(GLuint& program, const std::vector<GLuint>& shaders) {
            program = glCreateProgram();
            for (const auto& shader : shaders)
                glAttachShader(program, shader);

            glLinkProgram(program);

            for (const auto& shader : shaders) {
                glDetachShader(program, shader);
                glDeleteShader(shader);
            }

            GLint result = GL_FALSE;
            int infoLogLength;
            glGetProgramiv(program, GL_LINK_STATUS, &result);
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            if (!result) {
                std::vector<char> errorMessage(infoLogLength + 1);
                glGetProgramInfoLog(program, infoLogLength, NULL, &errorMessage[0]);
                std::string err(&errorMessage[0]);
                LOG_ERR("Error while compiling and linking the shader:\n", err, '\n');
                glDeleteProgram(program);
                return false;
            }

            return true;
        }

    } // namespace anonymous

    bool loadShaderFromText(Shader& shader, const ShaderFileDesc& desc) {
        std::string shaderSource;
        GLuint glShader;
        std::vector<GLuint> shaders;
        GLuint program;
        bool ret = true;

        if (desc.compute != "") {
            ret = loadShaderTextFile(shaderSource, desc.compute);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_COMPUTE_SHADER);
            shaders.push_back(glShader);
            ret = ret && createAndLinkProgram(program, shaders);
            if (!ret)
                return false;
            shader = Shader(program);
            return true;
        }

        if (desc.vertex != "") {
            ret = ret && loadShaderTextFile(shaderSource, desc.vertex);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_VERTEX_SHADER);
            if (ret)
                shaders.push_back(glShader);
        }
        if (desc.geometry != "") {
            ret = ret && loadShaderTextFile(shaderSource, desc.geometry);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_GEOMETRY_SHADER);
            if (ret)
                shaders.push_back(glShader);
        }
        if (desc.fragment != "") {
            ret = ret && loadShaderTextFile(shaderSource, desc.fragment);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_FRAGMENT_SHADER);
            if (ret)
                shaders.push_back(glShader);
        }
        if (!ret) {
            for (const auto& s : shaders)
                glDeleteShader(s);
            return false;
        }

        if (createAndLinkProgram(program, shaders)) {
            shader = Shader(program);
            return true;
        } else {
            return false;
        }
    }

} // namespace Progression
