#pragma once
#include "graphics/graphics_api.hpp"
#include "resource/resource.hpp"
#include <cstring>
#include <string>
#include <vector>

namespace Progression
{

enum ImageFlagBits
{
    IMAGE_CREATE_TEXTURE_ON_LOAD = 1 << 0,
    IMAGE_FREE_CPU_COPY_ON_LOAD  = 1 << 1,
    IMAGE_FLIP_VERTICALLY        = 1 << 2,
    IMAGE_GENERATE_MIPMAPS       = 1 << 3,
};

typedef uint32_t ImageFlags;

struct ImageCreateInfo : public ResourceCreateInfo
{
    std::string filename;
    std::vector< std::string > skyboxFilenames;
    std::string sampler        = "";
    ImageFlags flags           = 0;
    Gfx::PixelFormat dstFormat = Gfx::PixelFormat::INVALID;
};

class Image : public Resource 
{
public:
    Image() = default;
    Image( const Gfx::ImageDescriptor& desc );
    ~Image();
    Image( Image&& src );
    Image& operator=( Image&& src );

    static std::shared_ptr< Image > Load2DImageWithDefaultSettings( const std::string& filename );
    bool Load( ResourceCreateInfo* createInfo ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;
    
    bool Save( const std::string& filename, bool flipVertically = false ) const;
    void UploadToGpu();
    void ReadToCpu();
    void FreeGpuCopy();
    void FreeCpuCopy();

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

protected:
    Gfx::Texture m_texture;
    unsigned char* m_pixels = nullptr;
    ImageFlags m_flags      = 0;
};

} // namespace Progression
