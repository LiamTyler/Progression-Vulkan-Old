#pragma once

#include <unordered_map>

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

	class Shader : public NonCopyable {
	public:
		Shader();
        Shader(GLuint program, bool queryUniforms = true);
		~Shader();
        
        Shader(Shader&& shader);
        Shader& operator=(Shader&& shader);

        void free();
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
		std::unordered_map<std::string, GLuint> uniforms_;
	};

} // namespace Progression
