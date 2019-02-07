#include "graphics/shader.hpp"
#include "utils/logger.hpp"
#include <fstream>

namespace Progression {

	Shader::Shader() : program_((GLuint) -1) {
	}

	Shader::~Shader() {
        Free();
	}

    Shader::Shader(Shader&& shader) {
        *this = std::move(shader);
    }

    Shader& Shader::operator=(Shader&& shader) {
        program_ = std::move(shader.program_);
        shaders_ = std::move(shader.shaders_);
        attributeList_ = std::move(shader.attributeList_);
        uniformList_ = std::move(shader.uniformList_);

        shader.program_ = (GLuint) -1;

        return *this;
    }

    bool Shader::Load(const std::string& vertex_or_compute_fname, const std::string& frag_fname,
            const std::string& geom_fname)
    {
        if (frag_fname == "") {
            if (AttachShaderFromFile(GL_COMPUTE_SHADER, vertex_or_compute_fname) &&
                CreateAndLinkProgram())
                AutoDetectVariables();
            else
                return false;
        } else {
            if (geom_fname == "") {
                if (AttachShaderFromFile(GL_VERTEX_SHADER, vertex_or_compute_fname) &&
                    AttachShaderFromFile(GL_FRAGMENT_SHADER, frag_fname) && CreateAndLinkProgram())
                    AutoDetectVariables();
                else
                    return false;
            } else {
                if (AttachShaderFromFile(GL_VERTEX_SHADER, vertex_or_compute_fname) &&
                    AttachShaderFromFile(GL_GEOMETRY_SHADER, geom_fname) &&
                    AttachShaderFromFile(GL_FRAGMENT_SHADER, frag_fname) && CreateAndLinkProgram())
                    AutoDetectVariables();
                else
                    return false;

            }
        }

        return true;
    }

	bool Shader::AttachShaderFromString(GLenum shaderType, const std::string& source) {
		GLuint newShader = glCreateShader(shaderType);
		char const * sourcePointer = source.c_str();
		glShaderSource(newShader, 1, &sourcePointer, NULL);
		glCompileShader(newShader);

		GLint Result = GL_FALSE;
		int InfoLogLength;

		glGetShaderiv(newShader, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(newShader, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (!Result) {
			std::vector<char> ErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(newShader, InfoLogLength, NULL, &ErrorMessage[0]);
            std::string err(&ErrorMessage[0]);
            LOG_ERR("Error while loading shader:\n", err, '\n');
			return false;
		}

		shaders_.push_back(newShader);
		return true;
	}

	bool Shader::AttachShaderFromFile(GLenum shaderType, const std::string& filename) {
		std::ifstream in(filename);
		if (in.fail()) {
            LOG_ERR("Failed to open the shader file:", filename);
			return false;
		}
		std::string file, line;
		while (std::getline(in, line))
			file += line + '\n';
		in.close();
		return AttachShaderFromString(shaderType, file);
	}

    // Note: technically, if 
	bool Shader::CreateAndLinkProgram() {
		program_ = glCreateProgram();
		for (size_t i = 0; i < shaders_.size(); i++)
			glAttachShader(program_, shaders_[i]);

		glLinkProgram(program_);

		GLint Result = GL_FALSE;
		int InfoLogLength;
		glGetProgramiv(program_, GL_LINK_STATUS, &Result);
		glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (!Result) {
			std::vector<char> ErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(program_, InfoLogLength, NULL, &ErrorMessage[0]);
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

	void Shader::AutoDetectVariables() {
		Enable();
		GLint i;
		GLint count;
		GLint size;
		GLenum type;
		GLsizei attribBufSize;
		glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attribBufSize);
		GLchar* attribName = new GLchar[attribBufSize];
		GLsizei length;

		// attributes
		glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTES, &count);
		for (i = 0; i < count; i++) {
			glGetActiveAttrib(program_, (GLuint)i, attribBufSize,
				&length, &size, &type, attribName);
			std::string sName(attribName);
			// std::cout << "attrib: " << i << " = " << sName << std::endl;
			// attributeList_[sName] = i;
			AddAttribute(sName);
		}
		delete[] attribName;

		// uniforms
		GLsizei uniformBufSize;
		glGetProgramiv(program_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformBufSize);
		GLchar* uniformName = new GLchar[uniformBufSize];
		glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &count);
		// std::cout << "num uniforms: " << count << std::endl;
		for (i = 0; i < count; i++) {
			glGetActiveUniform(program_, (GLuint)i, uniformBufSize,
				&length, &size, &type, uniformName);
			std::string sName(uniformName);
			// std::cout << "uniform: " << i << " = " << sName << std::endl;
			// uniformList_[sName] = i;
			AddUniform(sName);
		}
		delete[] uniformName;
	}

    void Shader::Free() {
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

	void Shader::Enable() {
		glUseProgram(program_);
	}

	void Shader::Disable() {
		glUseProgram(0);
	}

	void Shader::AddAttribute(const std::string& attribute) {
		attributeList_[attribute] = glGetAttribLocation(program_, attribute.c_str());
	}

	void Shader::AddUniform(const std::string& uniform) {
		uniformList_[uniform] = glGetUniformLocation(program_, uniform.c_str());
	}

	GLuint Shader::GetUniform(const std::string& name) const {
		auto it = uniformList_.find(name);
		if (it == uniformList_.end())
			return (GLuint) -1;
		else
			return it->second;
	}

	GLuint Shader::GetAttribute(const std::string& name) const {
		auto it = attributeList_.find(name);
		if (it == attributeList_.end())
			return (GLuint) -1;
		else
			return it->second;
	}

	GLuint Shader::operator[] (const std::string& name) const {		
		std::unordered_map<std::string, GLuint>::const_iterator it = uniformList_.find(name);
        // assert(it != uniformList_.end());
		if (it != uniformList_.end())
			return it->second;
		it = attributeList_.find(name);
        // assert(it != attributeList_.end());
		if (it != attributeList_.end())
			return it->second;

		// std::cout << "uniform '" << name << "' not found!" << std::endl;
		return (GLuint) -1;
	}

} // namespace Progression
