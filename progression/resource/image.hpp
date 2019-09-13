#pragma once
#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"
#include <cstring>
#include <string>
#include <vector>

namespace Progression
{

enum ImageFlags
{
    IMAGE_CREATE_TEXTURE_ON_LOAD      = 1 << 0,
    IMAGE_FREE_CPU_COPY_ON_LOAD       = 1 << 1,
    IMAGE_FLIP_VERTICALLY             = 1 << 2,
    IMAGE_GENERATE_MIPMAPS_ON_CONVERT = 1 << 3,

    NUM_IMAGE_FLAGS                   = 4
};

inline ImageFlags operator|( ImageFlags a, ImageFlags b )
{
    return ImageFlags( int( a ) | int( b ) );
}
inline ImageFlags& operator|=( ImageFlags& a, ImageFlags b )
{
    return (ImageFlags&)( (int&) a |= (int) b );
}

struct ImageCreateInfo : public ResourceCreateInfo
{
    std::vector< std::string > filenames;
    std::string sampler = "";
    ImageFlags flags    = static_cast< ImageFlags >( 0 );
};

class Image : public Resource 
{
public:
    Image() = default;
    Image( const Gfx::ImageDescriptor& desc );
    ~Image();
    Image( Image&& src );
    Image& operator=( Image&& src );

    bool Load( ResourceCreateInfo* createInfo ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;

    bool Save( const std::string& filename, bool flipVertically = false ) const;

    Gfx::Texture* GetTexture();
    Gfx::ImageDescriptor GetDescriptor() const;
    Gfx::ImageType GetType() const;
    Gfx::PixelFormat GetPixelFormat() const;
    uint8_t GetMipLevels() const;
    uint8_t GetArrayLayers() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;
    unsigned char* GetPixels() const;
    size_t GetTotalImageBytes() const;
    ImageFlags GetImageFlags() const;

    Gfx::Sampler* sampler = nullptr;

protected:
    Gfx::Texture m_texture;
    unsigned char* m_pixels = nullptr;
    ImageFlags m_flags      = static_cast< ImageFlags >( 0 );
};

} // namespace Progression
