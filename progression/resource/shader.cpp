#include "resource/shader.hpp"
#include "utils/logger.hpp"

namespace Progression {

    Shader::Shader() :
        program_((GLuint) -1)
    {
    }

    Shader::Shader(GLuint program, bool _queryUniforms) :
        program_(program)
    {
        if (_queryUniforms)
            queryUniforms();
    }

    Shader::~Shader() {
        free();
    }

    Shader::Shader(Shader&& shader) {
        *this = std::move(shader);
    }

    Shader& Shader::operator=(Shader&& shader) {
        program_ = std::move(shader.program_);
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
        LOG("");
        delete[] uniformName;
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
