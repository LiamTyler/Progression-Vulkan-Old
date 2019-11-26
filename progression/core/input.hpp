#pragma once

#include "core/input_types.hpp"
#include "core/math.hpp"

struct lua_State;
void RegisterLuaFunctions_Input( lua_State* L );

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
    glm::vec2 GetMousePosition();
    glm::vec2 GetMouseChange();
    glm::vec2 GetScrollChange();

} // namespace Input
} // namespace Progression
