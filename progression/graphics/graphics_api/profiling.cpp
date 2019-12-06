#include "graphics/graphics_api/profiling.hpp"
#include "core/assert.hpp"
#include "graphics/vulkan.hpp"
#include <unordered_map>
#include <vector>

#define MAX_NUM_QUERIES 100

static std::vector< uint64_t > s_cpuQueries;
static VkQueryPool s_queryPool;
static float s_timestampPeriod;
static std::unordered_map< std::string, int > s_nameToIndexMap;
static uint32_t s_nextFreeIndex;

namespace Progression
{
namespace Gfx
{
namespace Profile
{

    void Init()
    {
        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType        = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext        = nullptr;
        createInfo.queryType    = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount   = MAX_NUM_QUERIES;
        VkResult res = vkCreateQueryPool( g_renderState.device.GetHandle(), &createInfo, nullptr, &s_queryPool );
        PG_ASSERT( res == VK_SUCCESS );

        s_cpuQueries.resize( MAX_NUM_QUERIES );
        s_timestampPeriod = g_renderState.physicalDeviceInfo.deviceProperties.limits.timestampPeriod;
        s_nextFreeIndex = 0;
    }

    void Shutdown()
    {
        vkDestroyQueryPool( g_renderState.device.GetHandle(), s_queryPool, nullptr );
    }

    void Reset( const CommandBuffer& cmdbuf )
    {
        vkCmdResetQueryPool( cmdbuf.GetHandle(), s_queryPool, 0, MAX_NUM_QUERIES );
        s_nextFreeIndex = 0;
    }

    void GetResults()
    {
        vkGetQueryPoolResults( g_renderState.device.GetHandle(), s_queryPool, 0, s_nextFreeIndex, s_cpuQueries.size() * sizeof( uint64_t ),
            s_cpuQueries.data(), sizeof( uint64_t ), VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT );
    }
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name )
    {
        s_nameToIndexMap[name] = s_nextFreeIndex++;
        vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_queryPool, s_nameToIndexMap[name] );
    }

    uint64_t GetTimestamp( const std::string& name )
    {
        PG_ASSERT( s_nameToIndexMap.find( name ) != s_nameToIndexMap.end() );
        return s_cpuQueries[s_nameToIndexMap[name]];
    }

    float GetDuration( const std::string& start, const std::string& end )
    {
        return ( GetTimestamp( end ) - GetTimestamp( start ) ) / s_timestampPeriod / 1e6f;
    }

} // namespace Profile
} // namespace Gfx
} // namespace Progression