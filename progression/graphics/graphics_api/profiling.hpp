#pragma once

#include "graphics/graphics_api/command_buffer.hpp"
#include <string>

namespace Progression
{
namespace Gfx
{
namespace Profile
{

    void Init();
    void Shutdown();

    void Reset( const CommandBuffer& cmdbuf );
    void GetResults();
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name );
    uint64_t GetTimestamp( const std::string& name );
    float GetDuration( const std::string& start, const std::string& end );

} // namespace Profile
} // namespace Gfx
} // namespace Progression