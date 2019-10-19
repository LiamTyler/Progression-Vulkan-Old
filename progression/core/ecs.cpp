#include "core/ecs.hpp"
#include "core/lua.hpp"

void RegisterLuaFunctions_ECS( lua_State* L )
{
    sol::state_view lua( L );
    sol::usertype< entt::registry > reg_type = lua.new_usertype< entt::registry >( "registry" );
    reg_type.set_function( "create", (entt::entity(entt::registry::*)() )&entt::registry::create );
}