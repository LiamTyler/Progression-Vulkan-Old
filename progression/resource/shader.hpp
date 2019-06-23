#pragma once

#include <unordered_map>

#include "resource/resource.hpp"
#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    typedef struct ShaderMetaData {
        bool operator==(const ShaderMetaData& desc) const;
        bool operator!=(const ShaderMetaData& desc) const;
        TimeStampedFile vertex;
        TimeStampedFile geometry;
        TimeStampedFile fragment;
        TimeStampedFile compute;
    } ShaderMetaData;

    void addShaderRootDir(ShaderMetaData& metaData, const std::string& root);

	class Shader : public NonCopyable, public Resource {
	public:
		Shader();
        Shader(const std::string& name, const ShaderMetaData& metaData);
        Shader(const std::string& name, GLuint program);
		~Shader();
        
        Shader(Shader&& shader);
        Shader& operator=(Shader&& shader);

        bool loadFromText();
        bool loadFromBinary(const char* binarySource, GLint len, GLenum format);
        char* getShaderBinary(GLint& len, GLenum& format);

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

        ShaderMetaData metaData;

	protected:
		GLuint program_;
		std::unordered_map<std::string, GLuint> uniforms_;
	};

} // namespace Progression
