#ifndef SRC_INCLUDE_GLSL_SHADER_H_
#define SRC_INCLUDE_GLSL_SHADER_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <map>
#include <vector>

class GLSLShader {
    public:
        GLSLShader();
        ~GLSLShader();
        void LoadFromString(GLenum shaderType, const std::string& source);
        void LoadFromFile(GLenum shaderType, const std::string& filename);
        void CreateAndLinkProgram();
        void Enable();
        void Disable();
        void AddAttribute(const std::string& attribute);
        void AddUniform(const std::string& uniform);
        GLuint operator[] (const std::string& name);
        void DeleteShaderProgram();

        GLuint GetProgram() { return program_; }
        void SetProgram(GLuint p) { program_ = p; }

    protected:
        GLuint program_;
        std::vector<GLuint> shaders_;
        std::map<std::string, GLuint> attributeList_;
        std::map<std::string, GLuint> uniformList_;
};

#endif  // SRC_INCLUDE_GLSL_SHADER_H_
