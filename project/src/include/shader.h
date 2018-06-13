#pragma once

#include "include/utils.h"

class Shader {
    public:
        Shader();
        Shader(const std::string& name);
        Shader(const std::string& name, const std::string& vertex_file,
                const std::string& frag_file);
        ~Shader();
        bool AttachShaderFromString(GLenum shaderType, const std::string& source);
        bool AttachShaderFromFile(GLenum shaderType, const std::string& fname);
        bool CreateAndLinkProgram();
        void AutoDetectVariables();
        void Enable();
        void Disable();
        void AddAttribute(const std::string& attribute);
        void AddUniform(const std::string& uniform);
        GLuint operator[] (const std::string& name) const;
        void DeleteShaderProgram();

        GLuint GetProgram() { return program_; }
        std::string ID() { return id_; }

    protected:
        std::string id_;
        GLuint program_;
        std::vector<GLuint> shaders_;
        std::unordered_map<std::string, GLuint> attributeList_;
        std::unordered_map<std::string, GLuint> uniformList_;
};
