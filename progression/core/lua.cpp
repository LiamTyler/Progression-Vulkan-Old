#include "core/lua.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/ecs.hpp"

namespace Progression
{

    sol::state g_LuaState;

    void RegisterTypesAndFunctionsToLua( sol::state& lua )
    {
        lua.open_libraries( sol::lib::base );
        RegisterLuaFunctions_Math( lua );
        RegisterLuaFunctions_Input( lua );
        RegisterLuaFunctions_ECS( lua ); // This must come before registering any components with the ECS

        auto luaTimeNamespace = lua["Time"].get_or_create< sol::table >();
        luaTimeNamespace["dt"] = 0;
    }

} // namespace Progression
