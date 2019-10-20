#include "core/ecs.hpp"
#include "core/lua.hpp"
#include "components/entity_metadata.hpp"

namespace Progression
{

    void RegisterLuaFunctions_ECS( lua_State* L )
    {
        sol::state_view lua( L );
        sol::usertype< entt::registry > reg_type = lua.new_usertype< entt::registry >( "registry" );
        reg_type.set_function( "create", (entt::entity(entt::registry::*)() )&entt::registry::create );
    }

    entt::entity GetEntityByName( entt::registry& registry, const std::string& name )
    {
        entt::entity e = entt::null;
        registry.view< NameComponent >().each([&]( const entt::entity& entity, const NameComponent& component )
        {
            if ( name == component.name )
            {
                e = entity;
            }
        });

        return e;
    }

} // namespace Progression
