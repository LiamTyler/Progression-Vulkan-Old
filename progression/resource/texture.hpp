#pragma once

#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"

namespace Progression
{

struct TextureCreateInfo : public ResourceCreateInfo
{
    std::string filename;
    ImageDesc imageDesc;
    std::string samplerName;
};

class Texture : public Resource
{
public:
    Texture();
    ~Texture() = default;

    Texture( Texture&& texture );
    Texture& operator=( Texture&& texture );

    bool Load( ResourceCreateInfo* createInfo ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;

    Gfx::Texture gfxTexture;
    Gfx::Sampler* sampler;
};

} // namespace Progression
