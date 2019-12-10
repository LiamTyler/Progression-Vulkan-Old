#include "graphics/graphics_api/profiling.hpp"
#include "core/assert.hpp"
#include "graphics/vulkan.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MAX_NUM_QUERIES 100

static std::vector< uint64_t > s_cpuQueries;
static VkQueryPool s_queryPool = VK_NULL_HANDLE;
static std::unordered_map< std::string, int > s_nameToIndexMap;
static uint32_t s_nextFreeIndex;
static double s_timestampToMillisInv;
std::string tmpFileName = "pg_profiling_log.txt";
static std::ofstream s_outputFile;

namespace Progression
{
namespace Gfx
{
namespace Profile
{

    bool Init()
    {
        #if !USING( PG_PROFILING )
            return true;
        #endif // #if !USING( SHIP_BUILD )
        VkQueryPoolCreateInfo createInfo = {};
        createInfo.sType        = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext        = nullptr;
        createInfo.queryType    = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount   = MAX_NUM_QUERIES;
        VkResult res = vkCreateQueryPool( g_renderState.device.GetHandle(), &createInfo, nullptr, &s_queryPool );
        PG_ASSERT( res == VK_SUCCESS );

        s_cpuQueries.resize( MAX_NUM_QUERIES );
        s_timestampToMillisInv = 1.0 / g_renderState.physicalDeviceInfo.deviceProperties.limits.timestampPeriod / 1e6;
        s_nextFreeIndex = 0;

        s_outputFile.open( tmpFileName );
        if ( !s_outputFile.is_open() )
        {
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        if ( s_queryPool != VK_NULL_HANDLE )
        {
            vkDestroyQueryPool( g_renderState.device.GetHandle(), s_queryPool, nullptr );
        }

        s_outputFile.close();

        // read the log file back in and calculate average durations between timestamps with same prefix
        // expects that the durations the user cares about come in pairs.
        // Eg: "Frame_Start" + "Frame_End", "ShadowPass_Start" + "ShadowPass_End", etc
        LOG( "Processing profiling timestamps..." );
        std::ifstream in( tmpFileName );
        struct Entry
        {
            double totalTime = 0;
            int count       = 0;
        };
        struct LogTimestamp
        {
            std::string name;
            double time;
        };
        std::unordered_map< std::string, Entry > pairings;
        
        std::vector< LogTimestamp > timestamps;
        
        std::string line;
        while ( std::getline( in, line ) )
        {
            // newline signifies end of frame. Calculate durations
            if ( line == "" )
            {
                std::sort( timestamps.begin(), timestamps.end(), []( const LogTimestamp& lhs, const LogTimestamp& rhs ) { return lhs.name < rhs.name; } );
                size_t numTimestamps = timestamps.size();
                for ( size_t i = 0; i < timestamps.size() && i != timestamps.size() - 1; i += 2 )
                {
                    auto pos = timestamps[i].name.find( '_' );
                    std::string first_prefix  = timestamps[i].name.substr( 0, pos );
                    std::string first_postfix = timestamps[i].name.substr( pos + 1 );
                    pos = timestamps[i + 1].name.find( '_' );
                    std::string second_prefix  = timestamps[i + 1].name.substr( 0, pos );
                    std::string second_postfix = timestamps[i + 1].name.substr( pos + 1 );

                    if ( first_prefix != second_prefix )
                    {
                        --i;
                        continue;
                    }
                    std::string entryName = first_prefix + ": " + second_postfix + " - " + first_postfix;
                    auto& entry      = pairings[entryName];
                    entry.count     += 1;
                    entry.totalTime += ( timestamps[i].time - timestamps[i + 1].time );                    
                }
                timestamps.clear();

                continue;
            }

            LogTimestamp t;
            std::stringstream ss( line );
            ss >> t.name >> t.time;
            auto pos = t.name.find( "_" );
            if ( pos != std::string::npos )
            {
                timestamps.push_back( t );
            }
        }

        if ( pairings.size() == 0 )
        {
            return;
        }

        LOG( "Profiling timestamps: " );
        for ( const auto& [ name, entry ] : pairings )
        {
            LOG( name, ": ", entry.totalTime * s_timestampToMillisInv / entry.count );
        }
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
        for ( const auto& [ name, index ] : s_nameToIndexMap )
        {
            s_outputFile << name << " " << s_cpuQueries[s_nameToIndexMap[name]] << "\n";
        }
        s_outputFile << "\n";
    }
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name )
    {
        s_nameToIndexMap[name] = s_nextFreeIndex++;
        vkCmdWriteTimestamp( cmdbuf.GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_queryPool, s_nameToIndexMap[name] );
    }

    uint64_t GetTimestamp( const std::string& name )
    {
        #if !USING( PG_PROFILING )
            return 0;
        #endif // #if !USING( SHIP_BUILD )
        PG_ASSERT( s_nameToIndexMap.find( name ) != s_nameToIndexMap.end() );
        return s_cpuQueries[s_nameToIndexMap[name]];
    }

    float GetDuration( const std::string& start, const std::string& end )
    {
        #if !USING( PG_PROFILING )
            return 0;
        #endif // #if !USING( SHIP_BUILD )
        return static_cast< float >( ( GetTimestamp( end ) - GetTimestamp( start ) ) * s_timestampToMillisInv );
    }

} // namespace Profile
} // namespace Gfx
} // namespace Progression