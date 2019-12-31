#pragma once

#include "imgui/imgui.h"
#include "graphics/graphics_api/command_buffer.hpp"

namespace Progression
{
namespace Gfx
{
namespace UIOverlay
{

    bool Init();
    void Shutdown();
    
    void Draw( CommandBuffer& cmdBuf );

} // namespace UIOverlay
} // namespace Gfx
} // namespace Progression