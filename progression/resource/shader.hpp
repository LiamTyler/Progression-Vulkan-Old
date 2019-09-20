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

    void Free();
    VkShaderModule GetNativeHandle() const;
    operator bool() const;


protected:
    VkShaderModule m_shaderModule;
    bool m_loaded;
};

} // namespace Progression
