#pragma once
#include <cstdint>
#include <vector>
#include "graphics_api/descriptor.hpp"

#define PG_MAX_GFX_TEXTURE_SLOTS 8

namespace Progression
{
namespace Gfx
{
    class Texture;

    void InitTextureManager();
    uint16_t GetOpenTextureSlot( Texture* texture );
    void FreeTextureSlot( uint16_t slot );
    void UpdateTextureDescriptors( const std::vector< DescriptorSet >& textureDescriptorSets );

} // namespace Gfx
} // namespace Progression