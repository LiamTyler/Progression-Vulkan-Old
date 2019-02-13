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

        /** \brief Load a the specified shader files and compile the shader for use. If the second
         *         and third arguments are empty strings, then the shader is assumed to be a
         *         compute shader, and the first argument is used as the filename for it.
         */
        bool load(
                const std::string& vertex_or_compute_fname,
                const std::string& frag_fname = "",
                const std::string& geom_fname = ""
                );
        void free();
		bool attachShaderFromString(GLenum shaderType, const std::string& source);
		bool attachShaderFromFile(GLenum shaderType, const std::string& fname);
		bool createAndLinkProgram();
		void queryUniforms();
		void enable() const;
		void disable() const;
		GLuint getUniform(const std::string& name) const;
		GLuint getAttribute(const std::string& name) const;
		GLuint getProgram() const { return program_; }

        void setUniform(const std::string& name, const bool data);
        void setUniform(const std::string& name, const int data);
        void setUniform(const std::string& name, const float data);
        void setUniform(const std::string& name, const glm::ivec2& data);
        void setUniform(const std::string& name, const glm::vec2& data);
        void setUniform(const std::string& name, const glm::vec3& data);
        void setUniform(const std::string& name, const glm::vec4& data);
        void setUniform(const std::string& name, const glm::mat3& data);
        void setUniform(const std::string& name, const glm::mat4& data);

        void setUniform(const std::string& name, const glm::mat4* data, int elements);
        void setUniform(const std::string& name, const glm::vec3* data, int elements);
        void setUniform(const std::string& name, const glm::vec4* data, int elements);

	protected:
		GLuint program_;
		std::vector<GLuint> shaders_;
		std::unordered_map<std::string, GLuint> uniforms_;
	};

} // namespace Progression
