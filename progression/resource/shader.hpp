#pragma once

#include <unordered_map>

#include "resource/resource.hpp"
#include "core/common.hpp"
#include "utils/noncopyable.hpp"
#include <unordered_set>

// #define PG_SHADER_WARNINGS

#ifndef PG_SHADER_WARNINGS
#define PG_SHADER_GETTER_CONST const
#else
#define PG_SHADER_GETTER_CONST
#endif

namespace Progression {

    class ShaderMetaData : public MetaData {
    public:
        bool operator==(const ShaderMetaData& desc) const;
        bool operator!=(const ShaderMetaData& desc) const;
        bool outOfDate(const ShaderMetaData& metaData) const;
        bool update();
        TimeStampedFile vertex;
        TimeStampedFile geometry;
        TimeStampedFile fragment;
        TimeStampedFile compute;
    };

	class Shader : public NonCopyable, public Resource {
	public:
		Shader();
        Shader(const std::string& name, const ShaderMetaData& metaData);
        Shader(const std::string& name, GLuint program);
		~Shader();
        
        Shader(Shader&& shader);
        Shader& operator=(Shader&& shader);

        bool load(MetaData* metaData = nullptr) override;
        bool readMetaData(std::istream& in) override;
        bool loadFromText();
        bool loadFromBinary(const char* binarySource, GLint len, GLenum format);
        std::vector<char> getShaderBinary(GLint& len, GLenum& format) const;
        ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) override;
        void move(Resource* resource) override;
        bool loadFromFastFile(std::ifstream& in) override;
        bool saveToFastFile(std::ofstream& out) const override;

        std::shared_ptr<Resource> needsReloading() override;

        void free();
		void queryUniforms();
		void enable() const;
		void disable() const;
		GLuint getUniform(const std::string& name) PG_SHADER_GETTER_CONST;
		GLuint getAttribute(const std::string& name) PG_SHADER_GETTER_CONST;
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

        // hash the shader warnings if they are enabled and only display them if the hash isnt
        // present so that the console doesnt overflow with the warnings
        #ifdef PG_SHADER_WARNINGS
        std::unordered_set<size_t> warnings_;
        #endif
	};

} // namespace Progression
