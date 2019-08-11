#pragma once

#include <unordered_map>

#include "core/common.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"
#include "utils/noncopyable.hpp"
#include <unordered_set>

#define PG_SHADER_WARNINGS

#ifndef PG_SHADER_WARNINGS
#define PG_SHADER_GETTER_CONST const
#else
#define PG_SHADER_GETTER_CONST
#endif

namespace Progression
{

struct ShaderCreateInfo : public ResourceCreateInfo
{
    std::string vertex;
    std::string geometry;
    std::string fragment;
    std::string compute;
};

class Shader : public Resource
{
public:
    Shader();
    ~Shader();

    Shader( Shader&& shader );
    Shader& operator=( Shader&& shader );

    bool Load( ResourceCreateInfo* createInfo );
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& out ) const override;
    bool Deserialize( std::ifstream& in ) override;
    bool Deserialize2( char*& buffer ) override;

    void Free();
    void QueryUniforms();
    void Enable() const;
    void Disable() const;
    GLuint GetUniform( const std::string& name ) PG_SHADER_GETTER_CONST;
    GLuint GetAttribute( const std::string& name ) PG_SHADER_GETTER_CONST;
    GLuint GetNativeHandle() const;

    void BindTexture( const Gfx::Texture& tex, const std::string& name, uint32_t index );

    void SetUniform( const std::string& name, const bool data );
    void SetUniform( const std::string& name, const int data );
    void SetUniform( const std::string& name, const float data );
    void SetUniform( const std::string& name, const glm::ivec2& data );
    void SetUniform( const std::string& name, const glm::vec2& data );
    void SetUniform( const std::string& name, const glm::vec3& data );
    void SetUniform( const std::string& name, const glm::vec4& data );
    void SetUniform( const std::string& name, const glm::mat3& data );
    void SetUniform( const std::string& name, const glm::mat4& data );
    void SetUniform( const std::string& name, const glm::mat4* data, int elements );
    void SetUniform( const std::string& name, const glm::vec3* data, int elements );
    void SetUniform( const std::string& name, const glm::vec4* data, int elements );

protected:
    bool LoadFromBinary( const char* binarySource, GLint len, GLenum format );
    std::vector< char > GetShaderBinary( GLint& len, GLenum& format ) const;

    GLuint m_program;
    std::unordered_map< std::string, GLuint > m_uniforms;

// hash the shader warnings if they are enabled and only display them if the hash isnt
// present so that the console doesnt overflow with the warnings
#ifdef PG_SHADER_WARNINGS
    std::unordered_set< size_t > m_warnings;
#endif
};

} // namespace Progression
