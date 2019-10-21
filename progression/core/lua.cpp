#include "core/lua.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/ecs.hpp"

#include "components/entity_metadata.hpp"
#include "components/script_component.hpp"

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

        sol::usertype< ScriptComponent > scriptComponent_type = lua.new_usertype< ScriptComponent >( "ScriptComponent" );
        scriptComponent_type["GetFunction"] = &ScriptComponent::GetFunction;
        REGISTER_COMPONENT_WITH_ECS( lua, ScriptComponent, &entt::registry::assign< ScriptComponent > );

        sol::usertype< NameComponent > nameComponent_type = lua.new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( lua, NameComponent, &entt::registry::assign< NameComponent > );
    }

} // namespace Progression
