#pragma once

#include "core/math.hpp"
#include "core/platform_defines.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>

namespace Progression
{

namespace Gfx
{
    enum class ShaderStage
    {
        VERTEX                  = 0x00000001,
        TESSELLATION_CONTROL    = 0x00000002,
        TESSELLATION_EVALUATION = 0x00000004,
        GEOMETRY                = 0x00000008,
        FRAGMENT                = 0x00000010,
        COMPUTE                 = 0x00000020,
    };
}

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
    std::vector< Gfx::DescriptorSetLayoutData > descriptorSetLayouts;
    std::vector< VkPushConstantRange > pushConstants;
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
    VkShaderModule GetHandle() const;
    operator bool() const;

    ShaderReflectInfo reflectInfo;
protected:
    VkShaderModule m_shaderModule;
};

} // namespace Progression
