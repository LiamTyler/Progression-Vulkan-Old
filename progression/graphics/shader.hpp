#pragma once

#include <vector>
#include <unordered_map>

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

	class Shader : public NonCopyable {
	public:
		Shader();
		~Shader();
        
        Shader(Shader&& shader);
        Shader& operator=(Shader&& shader);

        bool Load(const std::string& vertex_or_compute_fname, const std::string& frag_name = "");
		bool AttachShaderFromString(GLenum shaderType, const std::string& source);
		bool AttachShaderFromFile(GLenum shaderType, const std::string& fname);
		bool CreateAndLinkProgram();
		void AutoDetectVariables();
        void Free();
		void Enable();
		void Disable();
		void AddAttribute(const std::string& attribute);
		void AddUniform(const std::string& uniform);
		GLuint GetUniform(const std::string& name) const;
		GLuint GetAttribute(const std::string& name) const;
		GLuint operator[] (const std::string& name) const;

		GLuint getProgram() { return program_; }

	protected:
		GLuint program_;
		std::vector<GLuint> shaders_;
		std::unordered_map<std::string, GLuint> attributeList_;
		std::unordered_map<std::string, GLuint> uniformList_;
	};

} // namespace Progression
