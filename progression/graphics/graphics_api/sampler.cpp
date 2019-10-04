#include "graphics/graphics_api/sampler.hpp"

namespace Progression
{
namespace Gfx
{

    void Sampler::Free()
    {
    }

    FilterMode Sampler::GetMinFilter() const
    {
        return m_desc.minFilter;
    }

    FilterMode Sampler::GetMagFilter() const
    {
        return m_desc.magFilter;
    }

    WrapMode Sampler::GetWrapModeS() const
    {
        return m_desc.wrapModeS;
    }

    WrapMode Sampler::GetWrapModeT() const
    {
        return m_desc.wrapModeT;
    }

    WrapMode Sampler::GetWrapModeR() const
    {
        return m_desc.wrapModeR;
    }

    float Sampler::GetMaxAnisotropy() const
    {
        return m_desc.maxAnisotropy;
    }

    glm::vec4 Sampler::GetBorderColor() const
    {
        return m_desc.borderColor;
    }

    /*Sampler::operator bool() const
    {
        return m_nativeHandle != ~0u;
    }*/

} // namespace Gfx
} // namespace Progression