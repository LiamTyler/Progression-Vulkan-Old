#include "include/glsl_shader.h"
#include <iostream>
#include <fstream>
#include <iomanip>

GLSLShader::GLSLShader() {
    program_ = 0;
}

GLSLShader::~GLSLShader() {
    DeleteShaderProgram();
}

void GLSLShader::LoadFromString(GLenum shaderType, const std::string& source) {
    GLuint newShader = glCreateShader(shaderType);
    const char * sourcePointer = source.c_str();
    glShaderSource(newShader, 1, &sourcePointer, NULL);
    glCompileShader(newShader);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    glGetShaderiv(newShader, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(newShader, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        char* ErrorMessage = new char[InfoLogLength + 1];
        glGetProgramInfoLog(program_, InfoLogLength, NULL, ErrorMessage);
        std::cerr << "Error while loading shader: " << std::endl;
        std::cerr << ErrorMessage << std::endl;
        delete [] ErrorMessage;
        return;
    }
    shaders_.push_back(newShader);
}

void GLSLShader::LoadFromFile(GLenum shaderType, const std::string& filename) {
    std::ifstream in(filename);
    if (in.fail()) {
        std::cerr << "Failed to open the shader file: " << filename << std::endl;
        return;
    }
    std::string file, line;
    while (std::getline(in, line))
        file += line + '\n';
    in.close();
    LoadFromString(shaderType, file);
}

void GLSLShader::CreateAndLinkProgram() {
    program_ = glCreateProgram();
    for (int i = 0; i < shaders_.size(); i++)
        glAttachShader(program_, shaders_[i]);

    glLinkProgram(program_);

    GLint Result = GL_FALSE;
    int InfoLogLength;
    glGetProgramiv(program_, GL_LINK_STATUS, &Result);
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        char* ErrorMessage = new char[InfoLogLength + 1];
        glGetProgramInfoLog(program_, InfoLogLength, NULL, ErrorMessage);
        std::cerr << "Error while compiling and linking program: " << std::endl;
        std::cerr << ErrorMessage << std::endl;
        delete [] ErrorMessage;
    }

    for (int i = 0; i < shaders_.size(); i++) {
        glDetachShader(program_, shaders_[i]);
        glDeleteShader(shaders_[i]);
    }
    shaders_.clear();
}

void GLSLShader::Enable() {
    glUseProgram(program_);
}

void GLSLShader::Disable() {
    glUseProgram(0);
}

void GLSLShader::AddAttribute(const std::string& attribute) {
    attributeList_[attribute] = glGetAttribLocation(program_, attribute.c_str());
}

void GLSLShader::AddUniform(const std::string& uniform) {
    uniformList_[uniform] = glGetUniformLocation(program_, uniform.c_str());
}

GLuint GLSLShader::operator[] (const std::string& name) {
    std::map<std::string, GLuint>::iterator it;
    it = uniformList_.find(name);
    if (it != uniformList_.end())
        return it->second;
    it = attributeList_.find(name);
    if (it != attributeList_.end())
        return it->second;

    return -1;
}

void GLSLShader::DeleteShaderProgram() {
    glDeleteProgram(program_);
}
