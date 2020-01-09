#include "graphics/graphics_api/texture.hpp"
#include "core/assert.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
#include "graphics/texture_manager.hpp"

namespace Progression
{
namespace Gfx
{
  
    int SizeOfPixelFromat( const PixelFormat& format )
    {
        int size[] =
        {
            0,  // INVALID

            1,  // R8_UNORM
            1,  // R8_SNORM
            1,  // R8_UINT
            1,  // R8_SINT
            1,  // R8_SRGB

            2,  // R8_G8_UNORM
            2,  // R8_G8_SNORM
            2,  // R8_G8_UINT
            2,  // R8_G8_SINT
            2,  // R8_G8_SRGB

            3,  // R8_G8_B8_UNORM
            3,  // R8_G8_B8_SNORM
            3,  // R8_G8_B8_UINT
            3,  // R8_G8_B8_SINT
            3,  // R8_G8_B8_SRGB

            3,  // B8_G8_R8_UNORM
            3,  // B8_G8_R8_SNORM
            3,  // B8_G8_R8_UINT
            3,  // B8_G8_R8_SINT
            3,  // B8_G8_R8_SRGB

            4,  // R8_G8_B8_A8_UNORM
            4,  // R8_G8_B8_A8_SNORM
            4,  // R8_G8_B8_A8_UINT
            4,  // R8_G8_B8_A8_SINT
            4,  // R8_G8_B8_A8_SRGB

            4,  // B8_G8_R8_A8_UNORM
            4,  // B8_G8_R8_A8_SNORM
            4,  // B8_G8_R8_A8_UINT
            4,  // B8_G8_R8_A8_SINT
            4,  // B8_G8_R8_A8_SRGB

            2,  // R16_UNORM
            2,  // R16_SNORM
            2,  // R16_UINT
            2,  // R16_SINT
            2,  // R16_FLOAT

            4,  // R16_G16_UNORM
            4,  // R16_G16_SNORM
            4,  // R16_G16_UINT
            4,  // R16_G16_SINT
            4,  // R16_G16_FLOAT

            6,  // R16_G16_B16_UNORM
            6,  // R16_G16_B16_SNORM
            6,  // R16_G16_B16_UINT
            6,  // R16_G16_B16_SINT
            6,  // R16_G16_B16_FLOAT

            8,  // R16_G16_B16_A16_UNORM
            8,  // R16_G16_B16_A16_SNORM
            8,  // R16_G16_B16_A16_UINT
            8,  // R16_G16_B16_A16_SINT
            8,  // R16_G16_B16_A16_FLOAT

            4,  // R32_UINT
            4,  // R32_SINT
            4,  // R32_FLOAT

            8,  // R32_G32_UINT
            8,  // R32_G32_SINT
            8,  // R32_G32_FLOAT

            12, // R32_G32_B32_UINT
            12, // R32_G32_B32_SINT
            12, // R32_G32_B32_FLOAT

            16, // R32_G32_B32_A32_UINT
            16, // R32_G32_B32_A32_SINT
            16, // R32_G32_B32_A32_FLOAT

            2,  // DEPTH_16_UNORM
            4,  // DEPTH_32_FLOAT
            3,  // DEPTH_16_UNORM_STENCIL_8_UINT
            4,  // DEPTH_24_UNORM_STENCIL_8_UINT
            5,  // DEPTH_32_FLOAT_STENCIL_8_UINT

            1,  // STENCIL_8_UINT

            // Pixel size for the BC formats is the size of the 4x4 block, not per pixel
            8,  // BC1_RGB_UNORM
            8,  // BC1_RGB_SRGB
            8,  // BC1_RGBA_UNORM
            8,  // BC1_RGBA_SRGB
            16, // BC2_UNORM
            16, // BC2_SRGB
            16, // BC3_UNORM
            16, // BC3_SRGB
            8,  // BC4_UNORM
            8,  // BC4_SNORM
            16, // BC5_UNORM
            16, // BC5_SNORM
            16, // BC6H_UFLOAT
            16, // BC6H_SFLOAT
            16, // BC7_UNORM
            16, // BC7_SRGB
        };

        PG_ASSERT( static_cast< int >( format ) < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
        static_assert( ARRAY_COUNT( size ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return size[static_cast< int >( format )];
    }

    int NumComponentsInPixelFromat( const PixelFormat& format )
    {
        int components[] =
        {
            0,  // INVALID
            1,  // R8_UNORM
            1,  // R8_SNORM
            1,  // R8_UINT
            1,  // R8_SINT
            1,  // R8_SRGB
            2,  // R8_G8_UNORM
            2,  // R8_G8_SNORM
            2,  // R8_G8_UINT
            2,  // R8_G8_SINT
            2,  // R8_G8_SRGB
            3,  // R8_G8_B8_UNORM
            3,  // R8_G8_B8_SNORM
            3,  // R8_G8_B8_UINT
            3,  // R8_G8_B8_SINT
            3,  // R8_G8_B8_SRGB
            3,  // B8_G8_R8_UNORM
            3,  // B8_G8_R8_SNORM
            3,  // B8_G8_R8_UINT
            3,  // B8_G8_R8_SINT
            3,  // B8_G8_R8_SRGB
            4,  // R8_G8_B8_A8_UNORM
            4,  // R8_G8_B8_A8_SNORM
            4,  // R8_G8_B8_A8_UINT
            4,  // R8_G8_B8_A8_SINT
            4,  // R8_G8_B8_A8_SRGB
            4,  // B8_G8_R8_A8_UNORM
            4,  // B8_G8_R8_A8_SNORM
            4,  // B8_G8_R8_A8_UINT
            4,  // B8_G8_R8_A8_SINT
            4,  // B8_G8_R8_A8_SRGB
            1,  // R16_UNORM
            1,  // R16_SNORM
            1,  // R16_UINT
            1,  // R16_SINT
            1,  // R16_FLOAT
            2,  // R16_G16_UNORM
            2,  // R16_G16_SNORM
            2,  // R16_G16_UINT
            2,  // R16_G16_SINT
            2,  // R16_G16_FLOAT
            3,  // R16_G16_B16_UNORM
            3,  // R16_G16_B16_SNORM
            3,  // R16_G16_B16_UINT
            3,  // R16_G16_B16_SINT
            3,  // R16_G16_B16_FLOAT
            4,  // R16_G16_B16_A16_UNORM
            4,  // R16_G16_B16_A16_SNORM
            4,  // R16_G16_B16_A16_UINT
            4,  // R16_G16_B16_A16_SINT
            4,  // R16_G16_B16_A16_FLOAT
            1,  // R32_UINT
            1,  // R32_SINT
            1,  // R32_FLOAT
            2,  // R32_G32_UINT
            2,  // R32_G32_SINT
            2,  // R32_G32_FLOAT
            3,  // R32_G32_B32_UINT
            3,  // R32_G32_B32_SINT
            3,  // R32_G32_B32_FLOAT
            4,  // R32_G32_B32_A32_UINT
            4,  // R32_G32_B32_A32_SINT
            4,  // R32_G32_B32_A32_FLOAT
            1,  // DEPTH_16_UNORM
            1,  // DEPTH_32_FLOAT
            2,  // DEPTH_16_UNORM_STENCIL_8_UINT
            2,  // DEPTH_24_UNORM_STENCIL_8_UINT
            2,  // DEPTH_32_FLOAT_STENCIL_8_UINT
            1,  // STENCIL_8_UINT
            3,  // BC1_RGB_UNORM
            3,  // BC1_RGB_SRGB
            4,  // BC1_RGBA_UNORM
            4,  // BC1_RGBA_SRGB
            2,  // BC2_UNORM
            4,  // BC2_SRGB
            4,  // BC3_UNORM
            4,  // BC3_SRGB
            1,  // BC4_UNORM
            1,  // BC4_SNORM
            2,  // BC5_UNORM
            2,  // BC5_SNORM
            3,  // BC6H_UFLOAT
            3,  // BC6H_SFLOAT
            3,  // BC7_UNORM (can be RGB or RGBA)
            3,  // BC7_SRGB  (can be RGB or RGBA)
        };

        PG_ASSERT( static_cast< int >( format )   < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
        static_assert( ARRAY_COUNT( components ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return components[static_cast< int >( format )];
    }

    bool PixelFormatIsCompressed( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::BC1_RGB_UNORM ) <= f && f < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS );
    }

    bool PixelFormatIsDepthFormat( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::DEPTH_16_UNORM ) <= f &&
               f <= static_cast< int >( PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT );
    }

    bool PixelFormatIsStencil( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT ) <= f &&
               f <= static_cast< int >( PixelFormat::STENCIL_8_UINT );
    }

    const char* PixelFormatName( const PixelFormat& format )
    {
        const char* names[] =
        {
            "SOURCE_FORMAT",         // INVALID
            "R8_UNORM",              // R8_UNORM
            "R8_SNORM",              // R8_SNORM
            "R8_UINT",               // R8_UINT
            "R8_SINT",               // R8_SINT
            "R8_SRGB",               // R8_SRGB
            "R8_G8_UNORM",           // R8_G8_UNORM
            "R8_G8_SNORM",           // R8_G8_SNORM
            "R8_G8_UINT",            // R8_G8_UINT
            "R8_G8_SINT",            // R8_G8_SINT
            "R8_G8_SRGB",            // R8_G8_SRGB
            "R8_G8_B8_UNORM",        // R8_G8_B8_UNORM
            "R8_G8_B8_SNORM",        // R8_G8_B8_SNORM
            "R8_G8_B8_UINT",         // R8_G8_B8_UINT
            "R8_G8_B8_SINT",         // R8_G8_B8_SINT
            "R8_G8_B8_SRGB",         // R8_G8_B8_SRGB
            "B8_G8_R8_UNORM",        // B8_G8_R8_UNORM
            "B8_G8_R8_SNORM",        // B8_G8_R8_SNORM
            "B8_G8_R8_UINT",         // B8_G8_R8_UINT
            "B8_G8_R8_SINT",         // B8_G8_R8_SINT
            "B8_G8_R8_SRGB",         // B8_G8_R8_SRGB
            "R8_G8_B8_A8_UNORM",     // R8_G8_B8_A8_UNORM
            "R8_G8_B8_A8_SNORM",     // R8_G8_B8_A8_SNORM
            "R8_G8_B8_A8_UINT",      // R8_G8_B8_A8_UINT
            "R8_G8_B8_A8_SINT",      // R8_G8_B8_A8_SINT
            "R8_G8_B8_A8_SRGB",      // R8_G8_B8_A8_SRGB
            "B8_G8_R8_A8_UNORM",     // B8_G8_R8_A8_UNORM
            "B8_G8_R8_A8_SNORM",     // B8_G8_R8_A8_SNORM
            "B8_G8_R8_A8_UINT",      // B8_G8_R8_A8_UINT
            "B8_G8_R8_A8_SINT",      // B8_G8_R8_A8_SINT
            "B8_G8_R8_A8_SRGB",      // B8_G8_R8_A8_SRGB
            "R16_UNORM",             // R16_UNORM
            "R16_SNORM",             // R16_SNORM
            "R16_UINT",              // R16_UINT
            "R16_SINT",              // R16_SINT
            "R16_FLOAT",             // R16_FLOAT
            "R16_G16_UNORM",         // R16_G16_UNORM
            "R16_G16_SNORM",         // R16_G16_SNORM
            "R16_G16_UINT",          // R16_G16_UINT
            "R16_G16_SINT",          // R16_G16_SINT
            "R16_G16_FLOAT",         // R16_G16_FLOAT
            "R16_G16_B16_UNORM",     // R16_G16_B16_UNORM
            "R16_G16_B16_SNORM",     // R16_G16_B16_SNORM
            "R16_G16_B16_UINT",      // R16_G16_B16_UINT
            "R16_G16_B16_SINT",      // R16_G16_B16_SINT
            "R16_G16_B16_FLOAT",     // R16_G16_B16_FLOAT
            "R16_G16_B16_A16_UNORM", // R16_G16_B16_A16_UNORM
            "R16_G16_B16_A16_SNORM", // R16_G16_B16_A16_SNORM
            "R16_G16_B16_A16_UINT",  // R16_G16_B16_A16_UINT
            "R16_G16_B16_A16_SINT",  // R16_G16_B16_A16_SINT
            "R16_G16_B16_A16_FLOAT", // R16_G16_B16_A16_FLOAT
            "R32_UINT",              // R32_UINT
            "R32_SINT",              // R32_SINT
            "R32_FLOAT",             // R32_FLOAT
            "R32_G32_UINT",          // R32_G32_UINT
            "R32_G32_SINT",          // R32_G32_SINT
            "R32_G32_FLOAT",         // R32_G32_FLOAT
            "R32_G32_B32_UINT",      // R32_G32_B32_UINT
            "R32_G32_B32_SINT",      // R32_G32_B32_SINT
            "R32_G32_B32_FLOAT",     // R32_G32_B32_FLOAT
            "R32_G32_B32_A32_UINT",  // R32_G32_B32_A32_UINT
            "R32_G32_B32_A32_SINT",  // R32_G32_B32_A32_SINT
            "R32_G32_B32_A32_FLOAT", // R32_G32_B32_A32_FLOAT
            "DEPTH_16_UNORM",        // DEPTH_16_UNORM
            "DEPTH_32_FLOAT",        // DEPTH_32_FLOAT
            "DEPTH_16_UNORM_STENCIL_8_UINT", // DEPTH_16_UNORM_STENCIL_8_UINT
            "DEPTH_24_UNORM_STENCIL_8_UINT", // DEPTH_24_UNORM_STENCIL_8_UINT
            "DEPTH_32_FLOAT_STENCIL_8_UINT", // DEPTH_32_FLOAT_STENCIL_8_UINT
            "STENCIL_8_UINT", // STENCIL_8_UINT
            "BC1_RGB_UNORM",  // BC1_RGB_UNORM
            "BC1_RGB_SRGB",   // BC1_RGB_SRGB
            "BC1_RGBA_UNORM", // BC1_RGBA_UNORM
            "BC1_RGBA_SRGB",  // BC1_RGBA_SRGB
            "BC2_UNORM",      // BC2_UNORM
            "BC2_SRGB",       // BC2_SRGB
            "BC3_UNORM",      // BC3_UNORM
            "BC3_SRGB",       // BC3_SRGB
            "BC4_UNORM",      // BC4_UNORM
            "BC4_SNORM",      // BC4_SNORM
            "BC5_UNORM",      // BC5_UNORM
            "BC5_SNORM",      // BC5_SNORM
            "BC6H_UFLOAT",    // BC6H_UFLOAT
            "BC6H_SFLOAT",    // BC6H_SFLOAT
            "BC7_UNORM",      // BC7_UNORM
            "BC7_SRGB",       // BC7_SRGB
        };

        return names[static_cast< int >( format )];
    }

    PixelFormat PixelFormatFromString( const std::string& format )
    {
        std::unordered_map< std::string, PixelFormat > pixelFormatMap =
        {
            { "R8_UNORM", PixelFormat::R8_UNORM },
            { "R8_SNORM", PixelFormat::R8_SNORM },
            { "R8_UINT", PixelFormat::R8_UINT },
            { "R8_SINT", PixelFormat::R8_SINT },
            { "R8_SRGB", PixelFormat::R8_SRGB },
            { "R8_G8_UNORM", PixelFormat::R8_G8_UNORM },
            { "R8_G8_SNORM", PixelFormat::R8_G8_SNORM },
            { "R8_G8_UINT", PixelFormat::R8_G8_UINT },
            { "R8_G8_SINT", PixelFormat::R8_G8_SINT },
            { "R8_G8_SRGB", PixelFormat::R8_G8_SRGB },
            { "R8_G8_B8_UNORM", PixelFormat::R8_G8_B8_UNORM },
            { "R8_G8_B8_SNORM", PixelFormat::R8_G8_B8_SNORM },
            { "R8_G8_B8_UINT", PixelFormat::R8_G8_B8_UINT },
            { "R8_G8_B8_SINT", PixelFormat::R8_G8_B8_SINT },
            { "R8_G8_B8_SRGB", PixelFormat::R8_G8_B8_SRGB },
            { "B8_G8_R8_UNORM", PixelFormat::B8_G8_R8_UNORM },
            { "B8_G8_R8_SNORM", PixelFormat::B8_G8_R8_SNORM },
            { "B8_G8_R8_UINT", PixelFormat::B8_G8_R8_UINT },
            { "B8_G8_R8_SINT", PixelFormat::B8_G8_R8_SINT },
            { "B8_G8_R8_SRGB", PixelFormat::B8_G8_R8_SRGB },
            { "R8_G8_B8_A8_UNORM", PixelFormat::R8_G8_B8_A8_UNORM },
            { "R8_G8_B8_A8_SNORM", PixelFormat::R8_G8_B8_A8_SNORM },
            { "R8_G8_B8_A8_UINT", PixelFormat::R8_G8_B8_A8_UINT },
            { "R8_G8_B8_A8_SINT", PixelFormat::R8_G8_B8_A8_SINT },
            { "R8_G8_B8_A8_SRGB", PixelFormat::R8_G8_B8_A8_SRGB },
            { "B8_G8_R8_A8_UNORM", PixelFormat::B8_G8_R8_A8_UNORM },
            { "B8_G8_R8_A8_SNORM", PixelFormat::B8_G8_R8_A8_SNORM },
            { "B8_G8_R8_A8_UINT", PixelFormat::B8_G8_R8_A8_UINT },
            { "B8_G8_R8_A8_SINT", PixelFormat::B8_G8_R8_A8_SINT },
            { "B8_G8_R8_A8_SRGB", PixelFormat::B8_G8_R8_A8_SRGB },
            { "R16_UNORM", PixelFormat::R16_UNORM },
            { "R16_SNORM", PixelFormat::R16_SNORM },
            { "R16_UINT", PixelFormat::R16_UINT },
            { "R16_SINT", PixelFormat::R16_SINT },
            { "R16_FLOAT", PixelFormat::R16_FLOAT },
            { "R16_G16_UNORM", PixelFormat::R16_G16_UNORM },
            { "R16_G16_SNORM", PixelFormat::R16_G16_SNORM },
            { "R16_G16_UINT", PixelFormat::R16_G16_UINT },
            { "R16_G16_SINT", PixelFormat::R16_G16_SINT },
            { "R16_G16_FLOAT", PixelFormat::R16_G16_FLOAT },
            { "R16_G16_B16_UNORM", PixelFormat::R16_G16_B16_UNORM },
            { "R16_G16_B16_SNORM", PixelFormat::R16_G16_B16_SNORM },
            { "R16_G16_B16_UINT", PixelFormat::R16_G16_B16_UINT },
            { "R16_G16_B16_SINT", PixelFormat::R16_G16_B16_SINT },
            { "R16_G16_B16_FLOAT", PixelFormat::R16_G16_B16_FLOAT },
            { "R16_G16_B16_A16_UNORM", PixelFormat::R16_G16_B16_A16_UNORM },
            { "R16_G16_B16_A16_SNORM", PixelFormat::R16_G16_B16_A16_SNORM },
            { "R16_G16_B16_A16_UINT", PixelFormat::R16_G16_B16_A16_UINT },
            { "R16_G16_B16_A16_SINT", PixelFormat::R16_G16_B16_A16_SINT },
            { "R16_G16_B16_A16_FLOAT", PixelFormat::R16_G16_B16_A16_FLOAT },
            { "R32_UINT", PixelFormat::R32_UINT },
            { "R32_SINT", PixelFormat::R32_SINT },
            { "R32_FLOAT", PixelFormat::R32_FLOAT },
            { "R32_G32_UINT", PixelFormat::R32_G32_UINT },
            { "R32_G32_SINT", PixelFormat::R32_G32_SINT },
            { "R32_G32_FLOAT", PixelFormat::R32_G32_FLOAT },
            { "R32_G32_B32_UINT", PixelFormat::R32_G32_B32_UINT },
            { "R32_G32_B32_SINT", PixelFormat::R32_G32_B32_SINT },
            { "R32_G32_B32_FLOAT", PixelFormat::R32_G32_B32_FLOAT },
            { "R32_G32_B32_A32_UINT", PixelFormat::R32_G32_B32_A32_UINT },
            { "R32_G32_B32_A32_SINT", PixelFormat::R32_G32_B32_A32_SINT },
            { "R32_G32_B32_A32_FLOAT", PixelFormat::R32_G32_B32_A32_FLOAT },
            { "DEPTH_16_UNORM", PixelFormat::DEPTH_16_UNORM },
            { "DEPTH_32_FLOAT", PixelFormat::DEPTH_32_FLOAT },
            { "DEPTH_16_UNORM_STENCIL_8_UINT", PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT },
            { "DEPTH_24_UNORM_STENCIL_8_UINT", PixelFormat::DEPTH_24_UNORM_STENCIL_8_UINT },
            { "DEPTH_32_FLOAT_STENCIL_8_UINT", PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT },
            { "STENCIL_8_UINT", PixelFormat::STENCIL_8_UINT },
            { "BC1_RGB_UNORM", PixelFormat::BC1_RGB_UNORM },
            { "BC1_RGB_SRGB", PixelFormat::BC1_RGB_SRGB },
            { "BC1_RGBA_UNORM", PixelFormat::BC1_RGBA_UNORM },
            { "BC1_RGBA_SRGB", PixelFormat::BC1_RGBA_SRGB },
            { "BC2_UNORM", PixelFormat::BC2_UNORM },
            { "BC2_SRGB", PixelFormat::BC2_SRGB },
            { "BC3_UNORM", PixelFormat::BC3_UNORM },
            { "BC3_SRGB", PixelFormat::BC3_SRGB },
            { "BC4_UNORM", PixelFormat::BC4_UNORM },
            { "BC4_SNORM", PixelFormat::BC4_SNORM },
            { "BC5_UNORM", PixelFormat::BC5_UNORM },
            { "BC5_SNORM", PixelFormat::BC5_SNORM },
            { "BC6H_UFLOAT", PixelFormat::BC6H_UFLOAT },
            { "BC6H_SFLOAT", PixelFormat::BC6H_SFLOAT },
            { "BC7_UNORM", PixelFormat::BC7_UNORM },
            { "BC7_SRGB", PixelFormat::BC7_SRGB },
        };

        auto it = pixelFormatMap.find( format );
        if ( it == pixelFormatMap.end() )
        {
            return PixelFormat::INVALID;
        }

        return it->second;
    }

    uint32_t CalculateTotalTextureSize( const ImageDescriptor& desc )
    {
        PG_ASSERT( desc.depth == 1, "haven't added support for depth > 1 yet" );
        uint32_t totalSize = 0;
        int pixelFormatSize = SizeOfPixelFromat( desc.format );
        for ( uint32_t face = 0; face < desc.arrayLayers; ++face )
        {
            uint32_t w = desc.width;
            uint32_t h = desc.height;
            for ( uint32_t mip = 0; mip < desc.mipLevels; ++mip )
            {
                if ( PixelFormatIsCompressed( desc.format ) )
                {
                    uint32_t roundedWidth  = ( w + 3 ) & ~3;
                    uint32_t roundedHeight = ( h + 3 ) & ~3;
                    uint32_t numBlocksX    = roundedWidth  / 4;
                    uint32_t numBlocksY    = roundedHeight / 4;
                    totalSize += numBlocksX * numBlocksY * pixelFormatSize;
                }
                else
                {
                    totalSize += w * h * pixelFormatSize;
                }
                w = std::max( w >> 1, 1u );
                h = std::max( h >> 1, 1u );
            }
        }
        
        return totalSize;
    }

    void Texture::GenerateMipMaps()
    {
        PG_ASSERT( m_image != VK_NULL_HANDLE );
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties( g_renderState.physicalDeviceInfo.device, PGToVulkanPixelFormat( m_desc.format ), &formatProperties );
        PG_MAYBE_UNUSED( formatProperties );
        PG_ASSERT( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, "Mipmapping not supported on this gpu" );
        CommandBuffer cmdBuf = g_renderState.transientCommandPool.NewCommandBuffer();
        cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        for ( uint32_t face = 0; face < m_desc.arrayLayers; ++face )
        {
            barrier.subresourceRange.baseArrayLayer = face;
            int32_t mipWidth  = m_desc.width;
            int32_t mipHeight = m_desc.height;

            for ( uint32_t i = 1; i < m_desc.mipLevels; ++i )
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                cmdBuf.PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, barrier );

                VkImageBlit blit = {};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel       = i - 1;
                blit.srcSubresource.baseArrayLayer = face;
                blit.srcSubresource.layerCount     = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel       = i;
                blit.dstSubresource.baseArrayLayer = face;
                blit.dstSubresource.layerCount     = 1;

                vkCmdBlitImage( cmdBuf.GetHandle(),
                    m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR );

                barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                cmdBuf.PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier );

                if ( mipWidth > 1 ) mipWidth /= 2;
                if ( mipHeight > 1 ) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = m_desc.mipLevels - 1;
            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            cmdBuf.PipelineBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier );
        }

        cmdBuf.EndRecording();

        g_renderState.device.Submit( cmdBuf );
        g_renderState.device.WaitForIdle();

        cmdBuf.Free();
    }

    void Texture::Free()
    {
        PG_ASSERT( m_image     != VK_NULL_HANDLE );
        PG_ASSERT( m_imageView != VK_NULL_HANDLE );
        PG_ASSERT( m_memory    != VK_NULL_HANDLE );

        vkDestroyImage( m_device, m_image, nullptr );
        vkDestroyImageView( m_device, m_imageView, nullptr );
        vkFreeMemory( m_device, m_memory, nullptr );
        if ( m_textureSlot != PG_INVALID_TEXTURE_INDEX )
        {
            TextureManager::FreeSlot( m_textureSlot );
            m_textureSlot = PG_INVALID_TEXTURE_INDEX;
        }
        m_image       = VK_NULL_HANDLE;
        m_imageView   = VK_NULL_HANDLE;
        m_memory      = VK_NULL_HANDLE;
    }

    unsigned char* Texture::GetPixelData() const
    {
        PG_ASSERT( false );
        return nullptr;
    }

    ImageType Texture::GetType() const
    {
        return m_desc.type;
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        return m_desc.format;
    }

    uint32_t Texture::GetMipLevels() const
    {
        return m_desc.mipLevels;
    }

    uint32_t Texture::GetArrayLayers() const
    {
        return m_desc.arrayLayers;
    }

    uint32_t Texture::GetWidth() const
    {
        return m_desc.width;
    }

    uint32_t Texture::GetHeight() const
    {
        return m_desc.height;
    }

    uint32_t Texture::GetDepth() const
    {
        return m_desc.depth;
    }

    VkImage Texture::GetHandle() const
    {
        return m_image;
    }

    VkImageView Texture::GetView() const
    {
        return m_imageView;
    }

    VkDeviceMemory Texture::GetMemoryHandle() const
    {
        return m_memory;
    }

    uint16_t Texture::GetShaderSlot() const
    {
        return m_textureSlot;
    }

    Sampler* Texture::GetSampler() const
    {
        return m_sampler;
    }

    void Texture::SetSampler( Sampler* sampler )
    {
        PG_ASSERT( sampler );
        m_sampler = sampler;
        TextureManager::UpdateSampler( this );
    }

    Texture::operator bool() const
    {
        return m_image != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace Progression
