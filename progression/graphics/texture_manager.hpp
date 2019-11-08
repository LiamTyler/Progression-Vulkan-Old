#pragma once
#include <cstdint>
#include <vector>
#include "graphics_api/descriptor.hpp"

namespace Progression
{
namespace Gfx
{

class Texture;

namespace TextureManager
{

    void Init();
    void Shutdown();
    uint16_t GetOpenSlot( Texture* texture );
    void FreeSlot( uint16_t slot );
    void UpdateDescriptors( const std::vector< DescriptorSet >& textureDescriptorSets );

} // namespace TextureManager
} // namespace Gfx
} // namespace Progression
