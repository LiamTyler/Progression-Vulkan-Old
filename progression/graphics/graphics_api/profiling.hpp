#pragma once

#include "graphics/graphics_api/command_buffer.hpp"
#include "core/platform_defines.hpp"
#include <string>

namespace Progression
{
namespace Gfx
{
namespace Profile
{

    bool Init();
    void Shutdown();

    void Reset( const CommandBuffer& cmdbuf );
    void GetResults();
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name );
    uint64_t GetTimestamp( const std::string& name );
    float GetDuration( const std::string& start, const std::string& end );

} // namespace Profile
} // namespace Gfx
} // namespace Progression

#if !USING( SHIP_BUILD )

#define PG_PROFILING IN_USE

#else // #if !USING( SHIP_BUILD )

#define PG_PROFILING NOT_IN_USE

#endif // #else // #if !USING( SHIP_BUILD )

#if USING( PG_PROFILING )

#define PG_PROFILE_RESET( cmdbuf ) Progression::Gfx::Profile::Reset( cmdbuf );
#define PG_PROFILE_GET_RESULTS() Progression::Gfx::Profile::GetResults();
#define PG_PROFILE_TIMESTAMP( cmdbuf, name ) Progression::Gfx::Profile::Timestamp( cmdbuf, name );

#else // #if USING( PG_PROFILING )

#define PG_PROFILE_RESET( cmdbuf )
#define PG_PROFILE_GET_RESULTS()
#define PG_PROFILE_TIMESTAMP( cmdbuf, name )

#endif // #else // #if USING( PG_PROFILING )