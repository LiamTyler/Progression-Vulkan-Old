#pragma once

#include "core/math.hpp"
#include "core/platform_defines.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{

struct ShaderCreateInfo : public ResourceCreateInfo
{
    std::string filename;
};

class ShaderReflectInfo
{
public:
    ShaderReflectInfo() = default;

    std::string entryPoint;
    Gfx::ShaderStage stage;
    std::unordered_map< std::string, uint32_t > inputLocations;
    std::unordered_map< std::string, uint32_t > outputLocations;
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
    bool Deserialize( char*& buffer ) override;

    static ShaderReflectInfo Reflect( const uint32_t* spirv, size_t spirvSizeInBytes );
    VkPipelineShaderStageCreateInfo GetVkPipelineShaderStageCreateInfo() const;

    void Free();
    VkShaderModule GetNativeHandle() const;
    operator bool() const;

    ShaderReflectInfo reflectInfo;
protected:
    VkShaderModule m_shaderModule;
};

} // namespace Progression
