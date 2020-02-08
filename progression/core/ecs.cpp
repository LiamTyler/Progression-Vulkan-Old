#include "core/ecs.hpp"
#include "core/lua.hpp"
#include "components/entity_metadata.hpp"
#include "components/model_renderer.hpp"
#include "components/script_component.hpp"
#include "components/transform.hpp"

namespace Progression
{

    void RegisterLuaFunctions_ECS( lua_State* L )
    {
        sol::state_view lua( L );
        sol::usertype< entt::registry > reg_type = lua.new_usertype< entt::registry >( "registry" );
        reg_type.set_function( "create", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
        reg_type.set_function( "destroy", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
        lua.set_function( "GetEntityByName", &GetEntityByName );

        sol::usertype< Transform > transform_type = lua.new_usertype< Transform >( "Transform" );
        transform_type["position"] = &Transform::position;
        transform_type["rotation"] = &Transform::rotation;
        transform_type["scale"] = &Transform::scale;
        transform_type["GetModelMatrix"] = &Transform::GetModelMatrix;
        REGISTER_COMPONENT_WITH_ECS( lua, Transform,
            static_cast< Transform&( entt::registry::* )( const entt::entity )> ( &entt::registry::assign< Transform > ) );

        sol::usertype< ScriptComponent > scriptComponent_type = lua.new_usertype< ScriptComponent >( "ScriptComponent" );
        scriptComponent_type["GetFunction"] = &ScriptComponent::GetFunction;
        REGISTER_COMPONENT_WITH_ECS( lua, ScriptComponent,
            static_cast< ScriptComponent&( entt::registry::* )( const entt::entity ) >( &entt::registry::assign< ScriptComponent > ) );

        sol::usertype< NameComponent > nameComponent_type = lua.new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( lua, NameComponent,
            static_cast< NameComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::assign< NameComponent > ) );

        sol::usertype< ModelRenderer > modelRenderer_type = lua.new_usertype< ModelRenderer >( "ModelRenderer" );
        modelRenderer_type["model"]     = &ModelRenderer::model;
        modelRenderer_type["materials"] = &ModelRenderer::materials;
        REGISTER_COMPONENT_WITH_ECS( lua, ModelRenderer,
            static_cast< ModelRenderer&( entt::registry::* )( const entt::entity )> ( &entt::registry::assign< ModelRenderer > ) );
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
