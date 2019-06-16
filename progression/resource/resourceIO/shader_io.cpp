#include "resource/resourceIO/shader_io.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include "core/common.hpp"

namespace Progression {

    namespace {

        // TODO: faster way
        bool loadShaderTextFile(std::string& source, const std::string& filename) {
            std::ifstream in(filename);
            if (in.fail()) {
                LOG_ERR("Failed to open the shader file: ", filename);
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

    void addShaderRootDir(ShaderFileDesc& desc, const std::string& root) {
        if (desc.vertex.length() != 0)
            desc.vertex = root + desc.vertex;
        if (desc.geometry.length() != 0)
            desc.geometry = root + desc.geometry;
        if (desc.fragment.length() != 0)
            desc.fragment = root + desc.fragment;
        if (desc.compute.length() != 0)
            desc.compute = root + desc.compute;
    }

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
            shader = std::move(Shader(program));
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
            shader = std::move(Shader(program));
            return true;
        } else {
            return false;
        }
    }

    bool loadShaderFromBinary(Shader& shader, const char* binarySource, GLint len, GLenum format) {
        GLuint program = glCreateProgram();
        glProgramBinary(program, format, binarySource, len);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if( GL_FALSE == status ) {
            LOG("Failed to create program from the binary");
            return false;
        }
        shader = std::move(Shader(program));
        return true;
    }

    char* getShaderBinary(const Shader& shader, GLint& len, GLenum& format) {
        GLint formats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
        if (formats < 1) {
            LOG("No binary formats supported with this driver");
            return nullptr;
        }

        glGetProgramiv(shader.getProgram(), GL_PROGRAM_BINARY_LENGTH, &len);
        char* binary = new char[len];
        glGetProgramBinary(shader.getProgram(), len, NULL, &format, binary);

        return binary;
    }

    bool getShaderInfoFromResourceFile(std::string& name, ShaderFileDesc& desc, std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;

        // shader name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "vertex");
        if (!ss.eof())
            ss >> desc.vertex;
        PG_ASSERT(!in.fail() && !ss.fail());

        // geometry
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "geometry");
        if (!ss.eof())
            ss >> desc.geometry;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "fragment");
        if (!ss.eof())
            ss >> desc.fragment;
        PG_ASSERT(!in.fail() && !ss.fail());

        // compute
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "compute");
        if (!ss.eof())
            ss >> desc.compute;
        PG_ASSERT(!in.fail() && !ss.fail());

        return true;
    }

} // namespace Progression
