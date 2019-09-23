#pragma once

#include "graphics/graphics_api.hpp"
#include <tuple>
#include <assert.h>

namespace Progression
{
namespace Gfx
{

    constexpr VkShaderStageFlagBits PGToVulkanShaderStage( ShaderStage stage )
    {
        return static_cast< VkShaderStageFlagBits >( stage );
    }

    constexpr VkBlendFactor PGToVulkanBlendFactor( BlendFactor factor )
    {
        VkBlendFactor convert[] =
        {
            VK_BLEND_FACTOR_ZERO,                // ZERO
            VK_BLEND_FACTOR_ONE,                 // ONE
            VK_BLEND_FACTOR_SRC_COLOR,           // SRC_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // ONE_MINUS_SRC_COLOR
            VK_BLEND_FACTOR_SRC_ALPHA,           // SRC_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // ONE_MINUS_SRC_ALPHA
            VK_BLEND_FACTOR_DST_COLOR,           // DST_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, // ONE_MINUS_DST_COLOR
            VK_BLEND_FACTOR_DST_ALPHA,           // DST_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // ONE_MINUS_DST_ALPHA
            VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,  // SRC_ALPHA_SATURATE
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendFactor::NUM_BLEND_FACTORS ) );

        return convert[static_cast< int >( factor )];
    }

    constexpr VkBlendOp PGToOpenGLBlendEquation( BlendEquation eq )
    {
        VkBlendOp convert[] =
        {
            VK_BLEND_OP_ADD,              // ADD
            VK_BLEND_OP_SUBTRACT,         // SUBTRACT
            VK_BLEND_OP_REVERSE_SUBTRACT, // REVERSE_SUBTRACT
            VK_BLEND_OP_MIN,              // MIN
            VK_BLEND_OP_MAX,              // MAX
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendEquation::NUM_BLEND_EQUATIONS) );

        return convert[static_cast< int >( eq )];
    }

    constexpr VkFrontFace PGToVulkanWindingOrder( WindingOrder order )
    {
        return static_cast< VkFrontFace >( order );
    }

    constexpr VkCullModeFlagBits PGToVulkanCullFace( CullFace face )
    {
        return static_cast< VkCullModeFlagBits >( face );
    }

    constexpr VkPolygonMode PGToVulkanPolygonMode( PolygonMode mode )
    {
        return static_cast< VkPolygonMode >( mode );
    }

    
    //constexpr GLenum PGToOpenGLBufferType( BufferType type )
    //{
    //    GLenum convert[] =
    //    {
    //        GL_ARRAY_BUFFER,         // VERTEX
    //        GL_ELEMENT_ARRAY_BUFFER, // INDEX
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferType::NUM_BUFFER_TYPE) );

    //    return convert[static_cast< int >( type )];
    //}

    //constexpr GLenum PGToOpenGLBufferUsage( BufferUsage usage )
    //{
    //    GLenum convert[] =
    //    {
    //        GL_STATIC_DRAW,  // STATIC
    //        GL_DYNAMIC_DRAW, // DYNAMIC
    //        GL_STREAM_DRAW,  // STREAM
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferUsage::NUM_BUFFER_USAGE ) );

    //    return convert[static_cast< int >( usage )];
    //}

    constexpr VkFormat PGToVulkanBufferDataType( BufferDataType type )
    {
        VkFormat convert[] =
        {
            VK_FORMAT_UNDEFINED,        // INVALID

            VK_FORMAT_R8_UINT,          // UCHAR
            VK_FORMAT_R8G8_UINT,        // UCHAR2
            VK_FORMAT_R8G8B8_UINT,      // UCHAR3
            VK_FORMAT_R8G8B8A8_UINT,    // UCHAR4

            VK_FORMAT_R8_SINT,          // CHAR
            VK_FORMAT_R8G8_SINT,        // CHAR2
            VK_FORMAT_R8G8B8_SINT,      // CHAR3
            VK_FORMAT_R8G8B8A8_SINT,    // CHAR4

            VK_FORMAT_R8_UNORM,         // UCHAR_NORM
            VK_FORMAT_R8G8_UNORM,       // UCHAR2_NORM
            VK_FORMAT_R8G8B8_UNORM,     // UCHAR3_NORM
            VK_FORMAT_R8G8B8A8_UNORM,   // UCHAR4_NORM

            VK_FORMAT_R8_SNORM,         // CHAR_NORM
            VK_FORMAT_R8G8_SNORM,       // CHAR2_NORM
            VK_FORMAT_R8G8B8_SNORM,     // CHAR3_NORM
            VK_FORMAT_R8G8B8A8_SNORM,   // CHAR4_NORM

            VK_FORMAT_R16_UINT,          // USHORT
            VK_FORMAT_R16G16_UINT,       // USHORT2
            VK_FORMAT_R16G16B16_UINT,    // USHORT3
            VK_FORMAT_R16G16B16A16_UINT, // USHORT4

            VK_FORMAT_R16_SINT,          // SHORT
            VK_FORMAT_R16G16_SINT,       // SHORT2
            VK_FORMAT_R16G16B16_SINT,    // SHORT3
            VK_FORMAT_R16G16B16A16_SINT, // SHORT4

            VK_FORMAT_R16_UNORM,          // USHORT_NORM
            VK_FORMAT_R16G16_UNORM,       // USHORT2_NORM
            VK_FORMAT_R16G16B16_UNORM,    // USHORT3_NORM
            VK_FORMAT_R16G16B16A16_UNORM, // USHORT4_NORM

            VK_FORMAT_R16_SNORM,          // SHORT_NORM
            VK_FORMAT_R16G16_SNORM,       // SHORT2_NORM
            VK_FORMAT_R16G16B16_SNORM,    // SHORT3_NORM
            VK_FORMAT_R16G16B16A16_SNORM, // SHORT4_NORM

            VK_FORMAT_R16_SFLOAT,           // HALF
            VK_FORMAT_R16G16_SFLOAT,        // HALF2
            VK_FORMAT_R16G16B16_SFLOAT,     // HALF3
            VK_FORMAT_R16G16B16A16_SFLOAT,  // HALF4

            VK_FORMAT_R32_SFLOAT,           // FLOAT
            VK_FORMAT_R32G32_SFLOAT,        // FLOAT2
            VK_FORMAT_R32G32B32_SFLOAT,     // FLOAT3
            VK_FORMAT_R32G32B32A32_SFLOAT,  // FLOAT4

            VK_FORMAT_R32_UINT,           // UINT
            VK_FORMAT_R32G32_UINT,        // UINT2
            VK_FORMAT_R32G32B32_UINT,     // UINT3
            VK_FORMAT_R32G32B32A32_UINT,  // UINT4

            VK_FORMAT_R32_UINT,           // INT
            VK_FORMAT_R32G32_UINT,        // INT2
            VK_FORMAT_R32G32B32_UINT,     // INT3
            VK_FORMAT_R32G32B32A32_UINT,  // INT4
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferDataType::NUM_BUFFER_DATA_TYPE ) );

        return convert[static_cast< int >( type )];
    }

    constexpr VkVertexInputRate PGToVulkanVertexInputRate( VertexInputRate inputRate )
    {
        return static_cast< VkVertexInputRate >( inputRate );
    }

    constexpr VkPrimitiveTopology PGToVulkanPrimitiveType( PrimitiveType topology )
    {
        VkPrimitiveTopology convert[] =
        {
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST,     // POINTS
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST,      // LINES
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST ,     // LINE_STRIP
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // TRIANGLES
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // TRIANGLE_STRIP
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,   // TRIANGLE_FAN
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PrimitiveType::NUM_PRIMITIVE_TYPE ) );

        return convert[static_cast< int >( topology )];
    }

    //constexpr GLenum PGToOpenGLIndexType( IndexType indexType )
    //{
    //    GLenum convert[] = {
    //        GL_UNSIGNED_SHORT, // UNSIGNED_SHORT
    //        GL_UNSIGNED_INT,   // UNSIGNED_INT
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( IndexType::NUM_INDEX_TYPE ) );

    //    return convert[static_cast< int >( indexType )];
    //}

    //constexpr GLenum PGToOpenGLFilterMode( FilterMode mode )
    //{
    //    GLenum convert[] = {
    //        GL_NEAREST,                // NEAREST
    //        GL_LINEAR,                 // LINEAR
    //        GL_NEAREST_MIPMAP_NEAREST, // NEAREST_MIPMAP_NEAREST
    //        GL_LINEAR_MIPMAP_NEAREST,  // LINEAR_MIPMAP_NEAREST
    //        GL_NEAREST_MIPMAP_LINEAR,  // NEAREST_MIPMAP_LINEAR
    //        GL_LINEAR_MIPMAP_LINEAR,   // LINEAR_MIPMAP_LINEAR
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( FilterMode::NUM_FILTER_MODE ) );

    //    return convert[static_cast< int >( mode )];
    //}

    //constexpr GLenum PGToOpenGLWrapMode( WrapMode mode )
    //{
    //    GLenum convert[] = {
    //        GL_REPEAT,          // REPEAT
    //        GL_MIRRORED_REPEAT, // MIRRORED_REPEAT
    //        GL_CLAMP_TO_EDGE,   // CLAMP_TO_EDGE
    //        GL_CLAMP_TO_BORDER, // CLAMP_TO_BORDER
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( WrapMode::NUM_WRAP_MODE ) );

    //    return convert[static_cast< int >( mode )];
    //}

    //constexpr GLenum PGToOpenGLImageType( ImageType type )
    //{
    //    GLenum convert[] = {
    //        GL_TEXTURE_1D,              // TYPE_1D
    //        GL_TEXTURE_1D_ARRAY,        // TYPE_1D_ARRAY
    //        GL_TEXTURE_2D,              // TYPE_2D 
    //        GL_TEXTURE_2D_ARRAY,        // TYPE_2D_ARRAY
    //        GL_TEXTURE_CUBE_MAP,        // TYPE_2D_ARRAY
    //        GL_TEXTURE_CUBE_MAP_ARRAY,  // TYPE_CUBEMAP_ARRAY
    //        GL_TEXTURE_3D,              // TYPE_3D_ARRAY
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( ImageType::NUM_IMAGE_TYPES ) );

    //    return convert[static_cast< int >( type )];
    //}

    //constexpr GLenum PGToOpenGLPixelFormat( PixelFormat format )
    //{
    //    GLenum convert[] = {
    //        GL_R8,                // R8_UINT
    //        GL_R16F,              // R16_FLOAT
    //        GL_R32F,              // R32_FLOAT
    //        GL_RG8,               // R8_G8_UINT
    //        GL_RG16F,             // R16_G16_FLOAT
    //        GL_RG32F,             // R32_G32_FLOAT
    //        GL_RGB8,              // R8_G8_B8_UINT
    //        GL_RGB16F,            // R16_G16_B16_FLOAT
    //        GL_RGB32F,            // R32_G32_B32_FLOAT
    //        GL_RGBA8,             // R8_G8_B8_A8_UINT
    //        GL_RGBA16F,           // R16_G16_B16_A16_FLOAT
    //        GL_RGBA32F,           // R32_G32_B32_A32_FLOAT
    //        GL_SRGB8,             // R8_G8_B8_UINT_SRGB
    //        GL_SRGB8_ALPHA8,      // R8_G8_B8_A8_UINT_SRGB
    //        GL_R11F_G11F_B10F,    // R11_G11_B10_FLOAT
    //        GL_DEPTH_COMPONENT32, // DEPTH32_FLOAT
    //    };

    //    // static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

    //    return convert[static_cast< int >( format )];
    //}

    //inline std::tuple< GLenum, GLenum > PGToOpenGLFormatAndType( PixelFormat format )
    //{
    //    constexpr std::tuple< GLenum, GLenum > convert[] = {
    //        { GL_RED,  GL_UNSIGNED_BYTE },    // R8_UINT
    //        { GL_RED,  GL_HALF_FLOAT },       // R16_FLOAT
    //        { GL_RED,  GL_FLOAT },            // R32_FLOAT
    //        { GL_RG,   GL_UNSIGNED_BYTE },    // R8_G8_UINT
    //        { GL_RG,   GL_HALF_FLOAT },       // R16_G16_FLOAT
    //        { GL_RG,   GL_FLOAT },            // R32_G32_FLOAT
    //        { GL_RGB,  GL_UNSIGNED_BYTE },    // R8_G8_B8_UINT
    //        { GL_RGB,  GL_HALF_FLOAT },       // R16_G16_B16_FLOAT
    //        { GL_RGB,  GL_FLOAT },            // R32_G32_B32_FLOAT
    //        { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_UINT
    //        { GL_RGBA, GL_HALF_FLOAT },       // R16_G16_B16_A16_FLOAT
    //        { GL_RGBA, GL_FLOAT },            // R32_G32_B32_A32_FLOAT
    //        { GL_RGB,  GL_UNSIGNED_BYTE },    // R8_G8_B8_UINT_SRGB
    //        { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_UINT_SRGB
    //        { GL_RGBA, GL_UNSIGNED_BYTE },    // R11_G11_B10_FLOAT ??
    //        { GL_DEPTH_COMPONENT, GL_FLOAT }, // DEPTH32_FLOAT
    //    };

    //    // static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

    //    return convert[static_cast< int >( format )];
    //}

    //constexpr GLenum PGToOpenGLDepthCompareFunction( CompareFunction func )
    //{
    //    GLenum convert[] = {
    //        GL_NEVER,    // NEVER
    //        GL_LESS,     // LESS
    //        GL_LEQUAL,   // LEQUAL
    //        GL_EQUAL,    // EQUAL
    //        GL_GEQUAL,   // GEQUAL
    //        GL_GREATER,  // GREATER
    //        GL_NOTEQUAL, // NEQUAL
    //        GL_ALWAYS,   // ALWAYS
    //    };

    //    static_assert( ARRAY_COUNT( convert ) == static_cast< int >( CompareFunction::NUM_COMPARE_FUNCTION ) );

    //    return convert[static_cast< int >( func )];
    //}

    /*constexpr GLenum PGToOpenGLBitFieldMask( RenderTargetBuffers mask )
    {
        GLenum glMask = 0;
        if ( mask & RenderTargetBuffers::RENDER_TARGET_COLOR )
        {
            glMask |= GL_COLOR_BUFFER_BIT;
        }
        if ( mask & RenderTargetBuffers::RENDER_TARGET_DEPTH )
        {
            glMask |= GL_DEPTH_BUFFER_BIT;
        }
        if ( mask & RenderTargetBuffers::RENDER_TARGET_STENCIL )
        {
            glMask |= GL_STENCIL_BUFFER_BIT;
        }

        return glMask;
    }*/

} // namespace Gfx
} // namespace Progression
