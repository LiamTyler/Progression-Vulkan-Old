#include "graphics/shader.h"
#include <fstream>

namespace Progression {

	Shader::Shader() :
		program_(-1),
		loaded_(false)
	{
	}

	Shader::Shader(const std::string& vertex_file, const std::string& frag_file) :
		program_(-1),
		loaded_(false)
	{
		if (AttachShaderFromFile(GL_VERTEX_SHADER, vertex_file) && AttachShaderFromFile(GL_FRAGMENT_SHADER, frag_file)) {
			CreateAndLinkProgram();
			AutoDetectVariables();
		} else {
			std::cout << "failed to attach shaders" << std::endl;
		}
	}

	Shader::~Shader() {
        if (program_ != -1)
            glDeleteProgram(program_);
	}

    Shader::Shader(Shader&& shader) {
        *this = std::move(shader);
    }

    Shader& Shader::operator=(Shader&& shader) {
        program_ = std::move(shader.program_);
        shaders_ = std::move(shader.shaders_);
        attributeList_ = std::move(shader.attributeList_);
        uniformList_ = std::move(shader.uniformList_);
        loaded_ = std::move(shader.loaded_);

        shader.program_ = -1;

        return *this;
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
			printf("Error while loading shader:\n%s\n\n", &ErrorMessage[0]);
			return false;
		}

		shaders_.push_back(newShader);
		return true;
	}

	bool Shader::AttachShaderFromFile(GLenum shaderType, const std::string& filename) {
		std::ifstream in(filename);
		if (in.fail()) {
			std::cerr << "Failed to open the shader file: " << filename << std::endl;
			return false;
		}
		std::string file, line;
		while (std::getline(in, line))
			file += line + '\n';
		in.close();
		return AttachShaderFromString(shaderType, file);
	}

	bool Shader::CreateAndLinkProgram() {
		program_ = glCreateProgram();
		for (int i = 0; i < shaders_.size(); i++)
			glAttachShader(program_, shaders_[i]);

		glLinkProgram(program_);

		GLint Result = GL_FALSE;
		int InfoLogLength;
		glGetProgramiv(program_, GL_LINK_STATUS, &Result);
		glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (!Result) {
			std::vector<char> ErrorMessage(InfoLogLength + 1);
			glGetProgramInfoLog(program_, InfoLogLength, NULL, &ErrorMessage[0]);
			printf("Error while compiling and linking program:\n%s\n\n", &ErrorMessage[0]);
			return false;
		}

		for (int i = 0; i < shaders_.size(); i++) {
			glDetachShader(program_, shaders_[i]);
			glDeleteShader(shaders_[i]);
		}
		shaders_.clear();
		loaded_ = true;
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

		// bindings (cant get? only indices?)
		/*
		glGetProgramiv(prgm, GL_ACTIVE_UNIFORM_BLOCKS, &count);
		cout << "Active uniform blocks: " << count << endl;
		for (i = 0; i < count; i++) {
			GLint nameLen;
			glGetActiveUniformBlockiv(prgm, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLen);
			std::vector<GLchar> name(nameLen);
			glGetActiveUniformBlockName(prgm, i, nameLen, NULL, &name[0]);
			std::string sName;
			sName.assign(name.begin(), name.end() - 1);

			cout << "\t: Uniform Block: " << i << ", name: " << sName << endl;
		}
		*/
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
			return -1;
		else
			return it->second;
	}

	GLuint Shader::GetAttribute(const std::string& name) const {
		auto it = attributeList_.find(name);
		if (it == attributeList_.end())
			return -1;
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
		return -1;
	}

} // namespace Progression
