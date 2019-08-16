#pragma once

#include "graphics/graphics_api.hpp"
#include <tuple>

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

        return convert[static_cast< int >( eq )];
    }

    constexpr GLenum PGToOpenGLWindingOrder( WindingOrder order )
    {
        GLenum convert[] = {
            GL_CW,
            GL_CCW
        };

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

        return convert[static_cast< int >( face )];
    }

    constexpr GLenum PGToOpenGLBufferType( BufferType type )
    {
        GLenum convert[] = {
            GL_ARRAY_BUFFER,         // VERTEX
            GL_ELEMENT_ARRAY_BUFFER, // INDEX
        };

        return convert[static_cast< int >( type )];
    }

    constexpr GLenum PGToOpenGLBufferUsage( BufferUsage usage )
    {
        GLenum convert[] = {
            GL_STATIC_DRAW,  // STATIC
            GL_DYNAMIC_DRAW, // DYNAMIC
            GL_STREAM_DRAW,  // STREAM
        };

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

        return convert[static_cast< int >( topology )];
    }

    constexpr GLenum PGToOpenGLIndexType( IndexType indexType )
    {
        GLenum convert[] = {
            GL_UNSIGNED_SHORT, // UNSIGNED_SHORT
            GL_UNSIGNED_INT,   // UNSIGNED_INT
        };

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

        return convert[static_cast< int >( mode )];
    }

    constexpr GLenum PGToOpenGLTextureType( TextureType type )
    {
        GLenum convert[] = {
            GL_TEXTURE_2D, // TEXTURE2D
        };

        return convert[static_cast< int >( type )];
    }

    constexpr GLenum PGToOpenGLPixelFormat( PixelFormat format )
    {
        GLenum convert[] = {
            GL_R8,                // R8_Uint
            GL_R16F,              // R16_Float
            GL_R32F,              // R32_Float
            GL_RG8,               // R8_G8_Uint
            GL_RG16F,             // R16_G16_Float
            GL_RG32F,             // R32_G32_Float
            GL_RGB8,              // R8_G8_B8_Uint
            GL_RGB16F,            // R16_G16_B16_Float
            GL_RGB32F,            // R32_G32_B32_Float
            GL_RGBA8,             // R8_G8_B8_A8_Uint
            GL_RGBA16F,           // R16_G16_B16_A16_Float
            GL_RGBA32F,           // R32_G32_B32_A32_Float
            GL_SRGB8,             // R8_G8_B8_Uint_sRGB
            GL_SRGB8_ALPHA8,      // R8_G8_B8_A8_Uint_sRGB
            GL_R11F_G11F_B10F,    // R11_G11_B10_Float
            GL_DEPTH_COMPONENT32, // DEPTH32_Float
        };

        return convert[static_cast< int >( format )];
    }

    inline std::tuple< GLenum, GLenum > PGToOpenGLFormatAndType( PixelFormat format )
    {
        constexpr std::tuple< GLenum, GLenum > convert[] = {
            { GL_RED, GL_UNSIGNED_BYTE },     // R8_Uint
            { GL_RED, GL_HALF_FLOAT },        // R16_Float
            { GL_RED, GL_FLOAT },             // R32_Float
            { GL_RG, GL_UNSIGNED_BYTE },      // R8_G8_Uint
            { GL_RG, GL_HALF_FLOAT },         // R16_G16_Float
            { GL_RG, GL_FLOAT },              // R32_G32_Float
            { GL_RGB, GL_UNSIGNED_BYTE },     // R8_G8_B8_Uint
            { GL_RGB, GL_HALF_FLOAT },        // R16_G16_B16_Float
            { GL_RGB, GL_FLOAT },             // R32_G32_B32_Float
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_Uint
            { GL_RGBA, GL_HALF_FLOAT },       // R16_G16_B16_A16_Float
            { GL_RGBA, GL_FLOAT },            // R32_G32_B32_A32_Float
            { GL_RGB, GL_UNSIGNED_BYTE },     // R8_G8_B8_Uint_sRGB
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R8_G8_B8_A8_Uint_sRGB
            { GL_RGBA, GL_UNSIGNED_BYTE },    // R11_G11_B10_Float ??
            { GL_DEPTH_COMPONENT, GL_FLOAT }, // DEPTH32_Float
        };

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

        return convert[static_cast< int >( func )];
    }

} // namespace Gfx
} // namespace Progression
