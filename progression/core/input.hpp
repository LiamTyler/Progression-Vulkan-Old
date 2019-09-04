#pragma once

#include "core/input_types.hpp"
#include "core/math.hpp"

// TODO: Separate this better from GLFW, add game controller support, and concept of multiple
// controllers
namespace Progression
{
namespace Input
{

    void Init();
    void Free();
    void PollEvents();
    bool GetKeyDown( Key k );
    bool GetKeyUp( Key k );
    bool GetMouseButtonDown( MouseButton b );
    bool GetMouseButtonUp( MouseButton b );
    glm::ivec2 GetMousePosition();
    glm::ivec2 GetMouseChange();
    glm::ivec2 GetScrollChange();

} // namespace Input
} // namespace Progression
