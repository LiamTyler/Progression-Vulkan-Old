#include "graphics/texture_manager.hpp"
#include "core/assert.hpp"
#include "graphics/graphics_api/sampler.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/render_system.hpp"
#include "graphics/shader_c_shared/defines.h"
#include "graphics/vulkan.hpp"
#include "utils/logger.hpp"
#include <bitset>
#include <deque>

static uint16_t s_currentSlot;
static std::bitset< PG_MAX_NUM_TEXTURES > s_slotsInUse;
static std::deque< uint16_t > s_freeSlots;
static std::vector< VkWriteDescriptorSet > s_setWrites;
static std::vector< VkDescriptorImageInfo > s_imageInfos;
struct TexInfo
{
    VkImageView view;
    VkSampler sampler;
};

static std::vector< std::pair< uint16_t, TexInfo > > s_slotsAddedSinceLastUpdate;

namespace Progression
{
namespace Gfx
{
namespace TextureManager
{
    void Init()
    {
        s_setWrites.reserve( 256 );
        s_imageInfos.reserve( 256 );
        s_slotsAddedSinceLastUpdate.reserve( 256 );
        s_slotsInUse.reset();
        s_currentSlot = 0;
        s_freeSlots.clear();
    }

    void Shutdown()
    {
        s_setWrites.clear();
        s_imageInfos.clear();
        s_slotsAddedSinceLastUpdate.clear();
        s_slotsInUse.reset();
        s_currentSlot = 0;
        s_freeSlots.clear();
    }

    uint16_t GetOpenSlot( Texture* tex )
    {
        uint16_t openSlot;
        if ( s_freeSlots.empty() )
        {
            openSlot = s_currentSlot;
            ++s_currentSlot;
        }
        else
        {
            openSlot = s_freeSlots.back();
            s_freeSlots.pop_back();
        }
        PG_ASSERT( openSlot < PG_MAX_NUM_TEXTURES && !s_slotsInUse[openSlot] );
        s_slotsInUse[openSlot] = true;
        TexInfo info = { tex->GetView(), tex->GetSampler()->GetHandle() };
        s_slotsAddedSinceLastUpdate.emplace_back( openSlot, info );
        return openSlot;
    }

    void FreeSlot( uint16_t slot )
    {
        PG_ASSERT( s_slotsInUse[slot] );
        s_slotsInUse[slot] = false;
        s_freeSlots.push_front( slot );
    }

    void UpdateSampler( Texture* texture )
    {
        PG_ASSERT( texture && texture->GetShaderSlot() != PG_INVALID_TEXTURE_INDEX && s_slotsInUse[texture->GetShaderSlot()] );
        s_slotsAddedSinceLastUpdate.emplace_back( texture->GetShaderSlot(), texture );
    }

    void UpdateDescriptors( const std::vector< DescriptorSet >& textureDescriptorSets )
    {
        if ( s_slotsAddedSinceLastUpdate.empty() )
        {
            return;
        }
        s_setWrites.resize( s_slotsAddedSinceLastUpdate.size(), {} );
        s_imageInfos.resize( s_slotsAddedSinceLastUpdate.size() );
        for ( size_t i = 0; i < s_setWrites.size(); ++i )
        {
            s_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            s_imageInfos[i].imageView   = s_slotsAddedSinceLastUpdate[i].second.view;
            s_imageInfos[i].sampler     = s_slotsAddedSinceLastUpdate[i].second.sampler;

            s_setWrites[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            s_setWrites[i].dstBinding       = 0;
            s_setWrites[i].dstArrayElement  = static_cast< uint32_t >( s_slotsAddedSinceLastUpdate[i].first );
            s_setWrites[i].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            s_setWrites[i].descriptorCount  = 1;
            s_setWrites[i].pImageInfo       = &s_imageInfos[i];
        }

        for ( size_t imageIndex = 0; imageIndex < textureDescriptorSets.size(); ++imageIndex )
        {
            for ( size_t i = 0; i < s_setWrites.size(); ++i )
            {
                s_setWrites[i].dstSet = textureDescriptorSets[imageIndex].GetHandle();
            }
            vkUpdateDescriptorSets( g_renderState.device.GetHandle(), static_cast< uint32_t >( s_setWrites.size() ), s_setWrites.data(), 0, nullptr );
        }

        s_slotsAddedSinceLastUpdate.clear();
    }

} // namespace TextureManager
} // namespace Gfx
} // namespace Progression
