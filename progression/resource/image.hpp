#pragma once
#include "utils/noncopyable.hpp"
#include <cstring>
#include <string>

namespace Progression
{

enum class ImageType : uint8_t
{
    TYPE_1D            = 0,
    TYPE_1D_ARRAY      = 1,
    TYPE_2D            = 2,
    TYPE_2D_ARRAY      = 3,
    TYPE_CUBEMAP       = 4,
    TYPE_CUBEMAP_ARRAY = 5,
    TYPE_3D            = 6,

    NUM_IMAGE_TYPES
};

enum class PixelFormat : uint8_t
{
    R8_Uint                 = 0,
    R16_Float               = 1,
    R32_Float               = 2,

    R8_G8_Uint              = 3,
    R16_G16_Float           = 4,
    R32_G32_Float           = 5,

    R8_G8_B8_Uint           = 6,
    R16_G16_B16_Float       = 7,
    R32_G32_B32_Float       = 8,

    R8_G8_B8_A8_Uint        = 9,
    R16_G16_B16_A16_Float   = 10,
    R32_G32_B32_A32_Float   = 11,

    R8_G8_B8_Uint_sRGB      = 12,
    R8_G8_B8_A8_Uint_sRGB   = 13,

    R11_G11_B10_Float       = 14,

    DEPTH32_Float           = 15,

    NUM_PIXEL_FORMATS
};

int SizeOfPixelFromat( const PixelFormat& format );

class ImageDesc
{
public:
    ImageType type      = ImageType::NUM_IMAGE_TYPES;
    PixelFormat format  = PixelFormat::NUM_PIXEL_FORMATS;
    uint8_t mipLevels   = 1;
    uint8_t arrayLayers = 1;
    uint32_t width      = 0;
    uint32_t height     = 0;
    uint32_t depth      = 1;
};

class Image : public NonCopyable
{
public:
    Image() = default;
    Image( const ImageDesc& desc );
    ~Image();

    Image( Image&& src );
    Image& operator=( Image&& src );

    bool Load( const std::string& fname, bool flipVertically = true );
    bool Save( const std::string& fname, bool flipVertically = true ) const;

    // to / from  a binary file
    bool Serialize( std::ofstream& outFile ) const;
    bool Deserialize( char*& buffer );

    ImageDesc GetDescriptor() const;
    ImageType GetType() const;
    PixelFormat GetPixelFormat() const;
    uint8_t GetMipLevels() const;
    uint8_t GetArrayLayers() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;
    unsigned char* GetPixels() const;
    size_t GetTotalImageBytes() const;

protected:
    ImageDesc m_desc;
    unsigned char* m_pixels = nullptr;
};

} // namespace Progression
