#pragma once

#include <vector>
#include <unordered_map>

#include "core/common.hpp"

namespace Progression {

	class Shader {
	public:
		Shader();
		Shader(const std::string& vertex_file, const std::string& frag_file);
		~Shader();
        
        Shader(const Shader& shader) = delete;
        Shader& operator=(const Shader& shader) = delete;
        Shader(Shader&& shader);
        Shader& operator=(Shader&& shader);

		bool AttachShaderFromString(GLenum shaderType, const std::string& source);
		bool AttachShaderFromFile(GLenum shaderType, const std::string& fname);
		bool CreateAndLinkProgram();
		void AutoDetectVariables();
		void Enable();
		void Disable();
		void AddAttribute(const std::string& attribute);
		void AddUniform(const std::string& uniform);
		GLuint GetUniform(const std::string& name) const;
		GLuint GetAttribute(const std::string& name) const;
		GLuint operator[] (const std::string& name) const;

		GLuint getProgram() { return program_; }
		bool isLoaded() { return loaded_; }

	protected:
		GLuint program_;
		std::vector<GLuint> shaders_;
		std::unordered_map<std::string, GLuint> attributeList_;
		std::unordered_map<std::string, GLuint> uniformList_;
		bool loaded_;
	};

} // namespace Progression
