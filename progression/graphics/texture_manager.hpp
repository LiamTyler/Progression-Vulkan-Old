#pragma once
#include <cstdint>
#include <vector>
#include "graphics_api/descriptor.hpp"
#include "graphics/shader_c_shared/texture_defines.h"

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
