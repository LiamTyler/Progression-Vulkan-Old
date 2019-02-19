#include "graphics/shader.hpp"
#include "utils/logger.hpp"
#include <fstream>

namespace Progression {

    Shader::Shader() : program_((GLuint) -1) {
    }

    Shader::~Shader() {
        free();
    }

    Shader::Shader(Shader&& shader) {
        *this = std::move(shader);
    }

    Shader& Shader::operator=(Shader&& shader) {
        program_ = std::move(shader.program_);
        shaders_ = std::move(shader.shaders_);
        uniforms_ = std::move(shader.uniforms_);

        shader.program_ = (GLuint) -1;

        return *this;
    }

    bool Shader::load(
            const std::string& vertex_or_compute_fname,
            const std::string& frag_fname,
            const std::string& geom_fname)
    {
        bool ret = true;
        if (frag_fname == "" && geom_fname == "")
            ret = ret && attachShaderFromFile(GL_COMPUTE_SHADER, vertex_or_compute_fname);
        else
            ret = ret && attachShaderFromFile(GL_VERTEX_SHADER, vertex_or_compute_fname);

        if (frag_fname != "")
            ret = ret && attachShaderFromFile(GL_FRAGMENT_SHADER, frag_fname);

        if (geom_fname != "")
            ret = ret && attachShaderFromFile(GL_FRAGMENT_SHADER, frag_fname);

        ret = ret && createAndLinkProgram();

        if (ret)
            queryUniforms();

        return ret;
    }

    void Shader::free() {
        for (size_t i = 0; i < shaders_.size(); i++) {
            glDetachShader(program_, shaders_[i]);
            glDeleteShader(shaders_[i]);
        }
        shaders_.clear();
        if (program_ != (GLuint) -1) {
            glDeleteProgram(program_);
            program_ = (GLuint) -1;
        }
    }

    bool Shader::attachShaderFromString(GLenum shaderType, const std::string& source) {
        GLuint newShader = glCreateShader(shaderType);
        char const * sourcePointer = source.c_str();
        glShaderSource(newShader, 1, &sourcePointer, NULL);
        glCompileShader(newShader);

        GLint result = GL_FALSE;
        int infoLogLength;

        glGetShaderiv(newShader, GL_COMPILE_STATUS, &result);
        glGetShaderiv(newShader, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (!result) {
            std::vector<char> ErrorMessage(infoLogLength + 1);
            glGetShaderInfoLog(newShader, infoLogLength, NULL, &ErrorMessage[0]);
            std::string err(&ErrorMessage[0]);
            LOG_ERR("Error while loading shader:\n", err, '\n');
            return false;
        }

        shaders_.push_back(newShader);
        return true;
    }

    bool Shader::attachShaderFromFile(GLenum shaderType, const std::string& filename) {
        std::ifstream in(filename);
        if (in.fail()) {
            LOG_ERR("Failed to open the shader file:", filename);
            return false;
        }
        std::string file, line;
        while (std::getline(in, line))
            file += line + '\n';
        in.close();
        return attachShaderFromString(shaderType, file);
    }

    // Note: technically, if 
    bool Shader::createAndLinkProgram() {
        program_ = glCreateProgram();
        for (size_t i = 0; i < shaders_.size(); i++)
            glAttachShader(program_, shaders_[i]);

        glLinkProgram(program_);

        GLint result = GL_FALSE;
        int infoLogLength;
        glGetProgramiv(program_, GL_LINK_STATUS, &result);
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (!result) {
            std::vector<char> ErrorMessage(infoLogLength + 1);
            glGetProgramInfoLog(program_, infoLogLength, NULL, &ErrorMessage[0]);
            std::string err(&ErrorMessage[0]);
            LOG_ERR("Error while compiling and linking the shader:\n", err, '\n');
            return false;
        }

        for (size_t i = 0; i < shaders_.size(); i++) {
            glDetachShader(program_, shaders_[i]);
            glDeleteShader(shaders_[i]);
        }
        shaders_.clear();
        return true;
    }

    void Shader::queryUniforms() {
        GLsizei uniformBufSize;
        GLint count;
        glGetProgramiv(program_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformBufSize);
        GLchar* uniformName = new GLchar[uniformBufSize];
        glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &count);
        LOG("Num uniforms:",count);
        for (int i = 0; i < count; i++) {
            GLint size;
            GLenum type;
            GLsizei length;
            glGetActiveUniform(program_, (GLuint)i, uniformBufSize, &length, &size, &type, uniformName);

            std::string sName(uniformName);
            uniforms_[sName] = i;
            LOG("Uniform:",i,"=",sName);
        }
        delete[] uniformName;
        disable();
    }

    void Shader::enable() const {
        glUseProgram(program_);
    }

    void Shader::disable() const {
        glUseProgram(0);
    }

    GLuint Shader::getUniform(const std::string& name) const {
        auto it = uniforms_.find(name);
        if (it == uniforms_.end()) {
            LOG_WARN("Uniform:",name,"is not present in the shader, using -1 instead");
            return (GLuint) -1;
        } else {
            return it->second;
        }
    }

    GLuint Shader::getAttribute(const std::string& name) const {
        GLuint loc = glGetAttribLocation(program_, name.c_str());
        if (loc == (GLuint) -1)
            LOG_WARN("Attribute:",name,"is not present in the shader, using -1 instead");

        return loc;
    }

    void Shader::setUniform(const std::string& name, const bool data) {
        glUniform1i(getUniform(name), data);
    }

    void Shader::setUniform(const std::string& name, const int data) {
        glUniform1i(getUniform(name), data);
    }

    void Shader::setUniform(const std::string& name, const float data) {
        glUniform1f(getUniform(name), data);
    }

    void Shader::setUniform(const std::string& name, const glm::ivec2& data) {
        glUniform2i(getUniform(name), data.x, data.y);
    }

    void Shader::setUniform(const std::string& name, const glm::vec2& data) {
        glUniform2f(getUniform(name), data.x, data.y);
    }

    void Shader::setUniform(const std::string& name, const glm::vec3& data) {
        glUniform3f(getUniform(name), data.x, data.y, data.z);
    }

    void Shader::setUniform(const std::string& name, const glm::vec4& data) {
        glUniform4f(getUniform(name), data.x, data.y, data.z, data.w);
    }

    void Shader::setUniform(const std::string& name, const glm::mat3& data) {
        glUniformMatrix3fv(getUniform(name), 1, GL_FALSE, glm::value_ptr(data));
    }

    void Shader::setUniform(const std::string& name, const glm::mat4& data) {
        glUniformMatrix4fv(getUniform(name), 1, GL_FALSE, glm::value_ptr(data));
    }

    void Shader::setUniform(const std::string& name, const glm::mat4* data, int elements) {
        glUniformMatrix4fv(getUniform(name), elements, GL_FALSE, glm::value_ptr(data[0]));
    }

    void Shader::setUniform(const std::string& name, const glm::vec3* data, int elements) {
        glUniform3fv(getUniform(name), elements, glm::value_ptr(data[0]));
    }

    void Shader::setUniform(const std::string& name, const glm::vec4* data, int elements) {
        glUniform4fv(getUniform(name), elements, glm::value_ptr(data[0]));
    }

} // namespace Progression
