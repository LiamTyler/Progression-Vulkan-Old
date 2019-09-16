#pragma once

#include "graphics/graphics_api.hpp"
#include <tuple>
#include <assert.h>

namespace Progression
{
namespace Gfx
{

    constexpr GLenum PGToOpenGLBlendFactor( BlendFactor factor )
    {
        GLenum convert[] = {
            GL_ZERO,                // ZERO
            GL_ONE,                 // ONE
            GL_SRC_COLOR,           // SRC_COLOR
            GL_ONE_MINUS_SRC_COLOR, // ONE_MINUS_SRC_COLOR
            GL_SRC_ALPHA,           // SRC_ALPHA
            GL_ONE_MINUS_SRC_ALPHA, // ONE_MINUS_SRC_ALPHA
            GL_DST_COLOR,           // DST_COLOR
            GL_ONE_MINUS_DST_COLOR, // ONE_MINUS_DST_COLOR
            GL_DST_ALPHA,           // DST_ALPHA
            GL_ONE_MINUS_DST_ALPHA, // ONE_MINUS_DST_ALPHA
            GL_SRC_ALPHA_SATURATE,  // SRC_ALPHA_SATURATE
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendFactor::NUM_BLEND_FACTORS ) );

        return convert[static_cast< int >( factor )];
    }

    constexpr GLenum PGToOpenGLBlendEquation( BlendEquation eq )
    {
        GLenum convert[] = {
            GL_FUNC_ADD,                // ADD
            GL_FUNC_SUBTRACT,           // SUBTRACT
            GL_FUNC_REVERSE_SUBTRACT,   // REVERSE_SUBTRACT
            GL_MIN,                     // MIN
            GL_MAX,                     // MAX
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendEquation::NUM_BLEND_EQUATIONS) );

        return convert[static_cast< int >( eq )];
    }

    constexpr GLenum PGToOpenGLWindingOrder( WindingOrder order )
    {
        GLenum convert[] = {
            GL_CW,
            GL_CCW
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( WindingOrder::NUM_WINDING_ORDER ) );

        return convert[static_cast< int >( order )];
    }

    constexpr GLenum PGToOpenGLCullFace( CullFace face )
    {
        GLenum convert[] = {
            ~0u,                // NONE
            GL_FRONT,           // FRONT
            GL_BACK,            // BACK
            GL_FRONT_AND_BACK,  // FRONT_AND_BACK
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( CullFace::NUM_CULL_FACE ) );

        return convert[static_cast< int >( face )];
    }

    constexpr GLenum PGToOpenGLBufferType( BufferType type )
    {
        GLenum convert[] = {
            GL_ARRAY_BUFFER,         // VERTEX
            GL_ELEMENT_ARRAY_BUFFER, // INDEX
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferType::NUM_BUFFER_TYPE) );

        return convert[static_cast< int >( type )];
    }

    constexpr GLenum PGToOpenGLBufferUsage( BufferUsage usage )
    {
        GLenum convert[] = {
            GL_STATIC_DRAW,  // STATIC
            GL_DYNAMIC_DRAW, // DYNAMIC
            GL_STREAM_DRAW,  // STREAM
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferUsage::NUM_BUFFER_USAGE ) );

        return convert[static_cast< int >( usage )];
    }

    constexpr GLenum PGToOpenGLBufferDataType( BufferDataType dataType )
    {
        GLenum convert[] = {
            GL_HALF_FLOAT,     // FLOAT16
            GL_FLOAT,          // FLOAT32
            GL_BYTE,           // BYTE
            GL_UNSIGNED_BYTE,  // UNSIGNED_BYTE
            GL_SHORT,          // SHORT
            GL_UNSIGNED_SHORT, // UNSIGNED_SHORT
            GL_INT,            // INT
            GL_UNSIGNED_INT,   // UNSIGNED_INT
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferDataType::NUM_BUFFER_DATA_TYPE ) );

        return convert[static_cast< int >( dataType )];
    }

    constexpr GLenum PGToOpenGLPrimitiveType( PrimitiveType topology )
    {
        GLenum convert[] = {
            GL_POINTS,         // POINTS
            GL_LINES,          // LINES
            GL_LINE_STRIP,     // LINE_STRIP
            GL_TRIANGLES,      // TRIANGLES
            GL_TRIANGLE_STRIP, // TRIANGLE_STRIP
            GL_TRIANGLE_FAN,   // TRIANGLE_FAN
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PrimitiveType::NUM_PRIMITIVE_TYPE ) );

        return convert[static_cast< int >( topology )];
    }

    constexpr GLenum PGToOpenGLIndexType( IndexType indexType )
    {
        GLenum convert[] = {
            GL_UNSIGNED_SHORT, // UNSIGNED_SHORT
            GL_UNSIGNED_INT,   // UNSIGNED_INT
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( IndexType::NUM_INDEX_TYPE ) );

        return convert[static_cast< int >( indexType )];
    }

    constexpr GLenum PGToOpenGLFilterMode( FilterMode mode )
    {
        GLenum convert[] = {
            GL_NEAREST,                // NEAREST
            GL_LINEAR,                 // LINEAR
            GL_NEAREST_MIPMAP_NEAREST, // NEAREST_MIPMAP_NEAREST
            GL_LINEAR_MIPMAP_NEAREST,  // LINEAR_MIPMAP_NEAREST
            GL_NEAREST_MIPMAP_LINEAR,  // NEAREST_MIPMAP_LINEAR
            GL_LINEAR_MIPMAP_LINEAR,   // LINEAR_MIPMAP_LINEAR
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( FilterMode::NUM_FILTER_MODE ) );

        return convert[static_cast< int >( mode )];
    }

    constexpr GLenum PGToOpenGLWrapMode( WrapMode mode )
    {
        GLenum convert[] = {
            GL_REPEAT,          // REPEAT
            GL_MIRRORED_REPEAT, // MIRRORED_REPEAT
            GL_CLAMP_TO_EDGE,   // CLAMP_TO_EDGE
            GL_CLAMP_TO_BORDER, // CLAMP_TO_BORDER
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( WrapMode::NUM_WRAP_MODE ) );

        return convert[static_cast< int >( mode )];
    }

    constexpr GLenum PGToOpenGLImageType( ImageType type )
    {
        GLenum convert[] = {
            GL_TEXTURE_1D,              // TYPE_1D
            GL_TEXTURE_1D_ARRAY,        // TYPE_1D_ARRAY
            GL_TEXTURE_2D,              // TYPE_2D 
            GL_TEXTURE_2D_ARRAY,        // TYPE_2D_ARRAY
            GL_TEXTURE_CUBE_MAP,        // TYPE_2D_ARRAY
            GL_TEXTURE_CUBE_MAP_ARRAY,  // TYPE_CUBEMAP_ARRAY
            GL_TEXTURE_3D,              // TYPE_3D_ARRAY
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( ImageType::NUM_IMAGE_TYPES ) );

        return convert[static_cast< int >( type )];
    }

    constexpr GLenum PGToOpenGLPixelFormat( PixelFormat format )
    {
        GLenum convert[] = {
            GL_R8,                // R8_UINT
            GL_R16F,              // R16_FLOAT
            GL_R32F,              // R32_FLOAT
            GL_RG8,               // R8_G8_UINT
            GL_RG16F,             // R16_G16_FLOAT
            GL_RG32F,             // R32_G32_FLOAT
            GL_RGB8,              // R8_G8_B8_UINT
            GL_RGB16F,            // R16_G16_B16_FLOAT
            GL_RGB32F,            // R32_G32_B32_FLOAT
            GL_RGBA8,             // R8_G8_B8_A8_UINT
            GL_RGBA16F,           // R16_G16_B16_A16_FLOAT
            GL_RGBA32F,           // R32_G32_B32_A32_FLOAT
            GL_SRGB8,             // R8_G8_B8_UINT_SRGB
            GL_SRGB8_ALPHA8,      // R8_G8_B8_A8_UINT_SRGB
            GL_R11F_G11F_B10F,    // R11_G11_B10_FLOAT
            GL_DEPTH_COMPONENT32, // DEPTH32_FLOAT
        };

        // static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return convert[static_cast< int >( format )];
    }

    inline std::tuple< GLenum, GLenum > PGToOpenGLFormatAndType( PixelFormat format )
    {
        constexpr std::tuple< GLenum, GLenum > convert[] = {
            { GL_RED,  GL_UNSIGNED_BYTE },    // R8_UINT
            { GL_RED,  GL_HALF_FLOAT },       // R16_FLOAT
            { GL_RED,  GL_FLOAT },            // R32_FLOAT
            { GL_RG,   GL_UNSIGNED_BYTE },    // R8_G8_UINT
            { GL_RG,   GL_HALF_FLOAT },       // R16_G16_FLOAT
            { GL_RG,   GL_FLOAT },            // R32_G32_FLOAT
            { GL_RGB,  GL_UNSIGNED_BYTE },    // R8_G8_B8_UINT
            { GL_RGB,  GL_HALF_FLOAT },       // R16_G16_B16_FLOAT
            { GL_RGB,  GL_FLOAT },            // R32_G32_B32_FLOAT
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_UINT
            { GL_RGBA, GL_HALF_FLOAT },       // R16_G16_B16_A16_FLOAT
            { GL_RGBA, GL_FLOAT },            // R32_G32_B32_A32_FLOAT
            { GL_RGB,  GL_UNSIGNED_BYTE },    // R8_G8_B8_UINT_SRGB
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_UINT_SRGB
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R11_G11_B10_FLOAT ??
            { GL_DEPTH_COMPONENT, GL_FLOAT }, // DEPTH32_FLOAT
        };

        // static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return convert[static_cast< int >( format )];
    }

    constexpr GLenum PGToOpenGLDepthCompareFunction( CompareFunction func )
    {
        GLenum convert[] = {
            GL_NEVER,    // NEVER
            GL_LESS,     // LESS
            GL_LEQUAL,   // LEQUAL
            GL_EQUAL,    // EQUAL
            GL_GEQUAL,   // GEQUAL
            GL_GREATER,  // GREATER
            GL_NOTEQUAL, // NEQUAL
            GL_ALWAYS,   // ALWAYS
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( CompareFunction::NUM_COMPARE_FUNCTION ) );

        return convert[static_cast< int >( func )];
    }

    constexpr GLenum PGToOpenGLBitFieldMask( RenderTargetBuffers mask )
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
    }

} // namespace Gfx
} // namespace Progression
