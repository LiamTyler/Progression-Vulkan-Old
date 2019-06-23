#include "resource/shader.hpp"
#include "utils/logger.hpp"

namespace Progression {

    namespace {

        // TODO: faster read file
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

    bool ShaderMetaData::operator==(const ShaderMetaData& desc) const {
        return vertex == desc.vertex &&
                geometry == desc.geometry &&
                fragment == desc.fragment &&
                compute == desc.compute;
    }

    bool ShaderMetaData::operator!=(const ShaderMetaData& desc) const {
        return !(*this == desc);
    }

    void addShaderRootDir(ShaderMetaData& metaData, const std::string& root) {
        if (metaData.vertex.filename.length() != 0)
            metaData.vertex.filename = root + metaData.vertex.filename;
        if (metaData.geometry.filename.length() != 0)
            metaData.geometry.filename = root + metaData.geometry.filename;
        if (metaData.fragment.filename.length() != 0)
            metaData.fragment.filename = root + metaData.fragment.filename;
        if (metaData.compute.filename.length() != 0)
            metaData.compute.filename = root + metaData.compute.filename;
    }

    Shader::Shader() :
        Resource(""),
        program_((GLuint) -1)
    {
    }

    Shader::Shader(const std::string& _name, const ShaderMetaData& data) :
        Resource(_name),
        metaData(data)
    {
    }

    Shader::Shader(const std::string& _name, GLuint program) :
        Resource(_name),
        program_(program)
    {
        queryUniforms();
    }

    Shader::~Shader() {
        free();
    }

    Shader::Shader(Shader&& shader) {
        *this = std::move(shader);
    }

    Shader& Shader::operator=(Shader&& shader) {
        name      = std::move(shader.name);
        metaData  = std::move(shader.metaData);
        program_  = std::move(shader.program_);
        uniforms_ = std::move(shader.uniforms_);

        shader.program_ = (GLuint) -1;

        return *this;
    }

    void Shader::free() {
        if (program_ != (GLuint) -1) {
            glDeleteProgram(program_);
            program_ = (GLuint) -1;
        }
    }

    void Shader::queryUniforms() {
        GLsizei uniformBufSize;
        GLint count;
        glGetProgramiv(program_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformBufSize);
        GLchar* uniformName = new GLchar[uniformBufSize];
        glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &count);
        // LOG("Num uniforms:",count);
        for (int i = 0; i < count; i++) {
            GLint size;
            GLenum type;
            GLsizei length;
            glGetActiveUniform(program_, (GLuint)i, uniformBufSize, &length, &size, &type, uniformName);

            std::string sName(uniformName);
            // fix uniform arrays
            auto location = glGetUniformLocation(program_, sName.c_str());
            int len = sName.length();
            if (len > 3) {
                if (sName[len - 1] == ']' && sName[len - 3] == '[') {
                    uniforms_[sName.substr(0, len - 3)] = location;
                    LOG("Uniform:", location, "=", sName.substr(0, len - 3));
                }
            }
            uniforms_[sName] = location;
            LOG("Uniform:",location,"=",sName);
        }
        LOG("")
        delete[] uniformName;
    }

    void Shader::enable() const {
        glUseProgram(program_);
    }

    void Shader::disable() const {
        glUseProgram(0);
    }

    GLuint Shader::getUniform(const std::string& _name) const {
        auto it = uniforms_.find(_name);
        if (it == uniforms_.end()) {
            LOG_WARN("Uniform: ", _name, " is not present in the shader, using -1 instead");
            return (GLuint) -1;
        } else {
            return it->second;
        }
    }

    GLuint Shader::getAttribute(const std::string& _name) const {
        GLuint loc = glGetAttribLocation(program_, _name.c_str());
        if (loc == (GLuint) -1)
            LOG_WARN("Attribute: ", _name, " is not present in the shader, using -1 instead");

        return loc;
    }

    void Shader::setUniform(const std::string& _name, const bool data) {
        glUniform1i(getUniform(_name), data);
    }

    void Shader::setUniform(const std::string& _name, const int data) {
        glUniform1i(getUniform(_name), data);
    }

    void Shader::setUniform(const std::string& _name, const float data) {
        glUniform1f(getUniform(_name), data);
    }

    void Shader::setUniform(const std::string& _name, const glm::ivec2& data) {
        glUniform2i(getUniform(_name), data.x, data.y);
    }

    void Shader::setUniform(const std::string& _name, const glm::vec2& data) {
        glUniform2f(getUniform(_name), data.x, data.y);
    }

    void Shader::setUniform(const std::string& _name, const glm::vec3& data) {
        glUniform3f(getUniform(_name), data.x, data.y, data.z);
    }

    void Shader::setUniform(const std::string& _name, const glm::vec4& data) {
        glUniform4f(getUniform(_name), data.x, data.y, data.z, data.w);
    }

    void Shader::setUniform(const std::string& _name, const glm::mat3& data) {
        glUniformMatrix3fv(getUniform(_name), 1, GL_FALSE, glm::value_ptr(data));
    }

    void Shader::setUniform(const std::string& _name, const glm::mat4& data) {
        glUniformMatrix4fv(getUniform(_name), 1, GL_FALSE, glm::value_ptr(data));
    }

    void Shader::setUniform(const std::string& _name, const glm::mat4* data, int elements) {
        glUniformMatrix4fv(getUniform(_name), elements, GL_FALSE, glm::value_ptr(data[0]));
    }

    void Shader::setUniform(const std::string& _name, const glm::vec3* data, int elements) {
        glUniform3fv(getUniform(_name), elements, glm::value_ptr(data[0]));
    }

    void Shader::setUniform(const std::string& _name, const glm::vec4* data, int elements) {
        glUniform4fv(getUniform(_name), elements, glm::value_ptr(data[0]));
    }

    bool Shader::loadFromText() {
        std::string shaderSource;
        GLuint glShader;
        std::vector<GLuint> shaders;
        GLuint program;
        bool ret = true;

        free();

        if (metaData.compute.filename != "") {
            ret = loadShaderTextFile(shaderSource, metaData.compute.filename);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_COMPUTE_SHADER);
            shaders.push_back(glShader);
            ret = ret && createAndLinkProgram(program, shaders);
            if (!ret)
                return false;
            program_ = program;
            return true;
        }

        if (metaData.vertex.filename != "") {
            ret = ret && loadShaderTextFile(shaderSource, metaData.vertex.filename);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_VERTEX_SHADER);
            if (ret)
                shaders.push_back(glShader);
        }
        if (metaData.geometry.filename != "") {
            ret = ret && loadShaderTextFile(shaderSource, metaData.geometry.filename);
            ret = ret && compileShader(glShader, shaderSource.c_str(), GL_GEOMETRY_SHADER);
            if (ret)
                shaders.push_back(glShader);
        }
        if (metaData.fragment.filename != "") {
            ret = ret && loadShaderTextFile(shaderSource, metaData.fragment.filename);
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
            program_ = program;
            queryUniforms();
            return true;
        } else {
            return false;
        }
    }

    bool Shader::loadFromBinary(const char* binarySource, GLint len, GLenum format) {
        free();
        GLuint program = glCreateProgram();
        glProgramBinary(program, format, binarySource, len);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if( GL_FALSE == status ) {
            LOG("Failed to create program from the binary");
            return false;
        }
        program_ = program;
        return true;
    }

    char* Shader::getShaderBinary(GLint& len, GLenum& format) {
        GLint formats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
        if (formats < 1) {
            LOG("No binary formats supported with this driver");
            return nullptr;
        }

        glGetProgramiv(program_, GL_PROGRAM_BINARY_LENGTH, &len);
        char* binary = new char[len];
        glGetProgramBinary(program_, len, NULL, &format, binary);

        return binary;
    }

} // namespace Progression
