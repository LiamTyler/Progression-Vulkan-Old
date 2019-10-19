#include "core/lua.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/ecs.hpp"

void RegisterTypesAndFunctionsToLua( sol::state& lua )
{
    RegisterLuaFunctions_Math( lua );
    RegisterLuaFunctions_Input( lua );
    RegisterLuaFunctions_ECS( lua ); // This must come before registering any components with the ECS
}