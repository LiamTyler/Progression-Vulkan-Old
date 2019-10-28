#include "graphics/texture_manager.hpp"
#include "core/assert.hpp"
#include "graphics/graphics_api/sampler.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/render_system.hpp"
#include "graphics/vulkan.hpp"
#include "utils/logger.hpp"
#include <bitset>
#include <deque>

static uint16_t s_currentSlot;
static std::bitset< PG_MAX_NUM_TEXTURES > s_slotsInUse;
static std::deque< uint16_t > s_freeSlots;
static std::vector< VkWriteDescriptorSet > s_setWrites;
static std::vector< VkDescriptorImageInfo > s_imageInfos;
static std::vector< std::pair< uint16_t, VkImageView > > s_slotsAddedSinceLastUpdate;

namespace Progression
{
namespace Gfx
{

    void InitTextureManager()
    {
        s_setWrites.reserve( 128 );
        s_imageInfos.reserve( 128 );
        s_slotsAddedSinceLastUpdate.reserve( 128 );
        s_slotsInUse.reset();
        s_currentSlot = 0;
        s_freeSlots.clear();
    }

    uint16_t GetOpenTextureSlot( Texture* texture )
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
        s_slotsAddedSinceLastUpdate.emplace_back( openSlot, texture->GetView() );
        return openSlot;
    }

    void FreeTextureSlot( uint16_t slot )
    {
        PG_ASSERT( s_slotsInUse[slot] );
        s_slotsInUse[slot] = false;
        s_freeSlots.push_front( slot );
    }

    void UpdateTextureDescriptors( const std::vector< DescriptorSet >& textureDescriptorSets )
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
            s_imageInfos[i].imageView   = s_slotsAddedSinceLastUpdate[i].second;
            s_imageInfos[i].sampler     = RenderSystem::GetSampler( "nearest_clamped" )->GetHandle();

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

} // namespace Gfx
} // namespace Progression
