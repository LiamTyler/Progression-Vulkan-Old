#pragma once

#include "glm/vec4.hpp"

namespace Progression
{
namespace Gfx
{

    enum class FilterMode
    {
        NEAREST                = 0,
        LINEAR                 = 1,
        NEAREST_MIPMAP_NEAREST = 2,
        LINEAR_MIPMAP_NEAREST  = 3,
        NEAREST_MIPMAP_LINEAR  = 4,
        LINEAR_MIPMAP_LINEAR   = 5,

        NUM_FILTER_MODE
    };

    enum class WrapMode
    {
        REPEAT          = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE   = 2,
        CLAMP_TO_BORDER = 3,

        NUM_WRAP_MODE
    };

    class SamplerDescriptor
    {
    public:
        FilterMode minFilter  = FilterMode::LINEAR;
        FilterMode magFilter  = FilterMode::LINEAR;
        WrapMode wrapModeS    = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeT    = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeR    = WrapMode::CLAMP_TO_EDGE;
        float maxAnisotropy   = 1.0f;
        glm::vec4 borderColor = glm::vec4( 0 );
    };

    class Sampler
    {
    public:
        void Free();

        FilterMode GetMinFilter() const;
        FilterMode GetMagFilter() const;
        WrapMode GetWrapModeS() const;
        WrapMode GetWrapModeT() const;
        WrapMode GetWrapModeR() const;
        float GetMaxAnisotropy() const;
        glm::vec4 GetBorderColor() const;
        // operator bool() const;

    private:
        SamplerDescriptor m_desc;
        // GLuint m_nativeHandle = ~0u;
    };

} // namespace Gfx
} // namespace Progression