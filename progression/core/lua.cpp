#include "core/lua.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/ecs.hpp"

#include "core/camera.hpp"
#include "core/scene.hpp"
#include "components/entity_metadata.hpp"
#include "components/script_component.hpp"
#include "components/transform.hpp"

namespace Progression
{

    sol::state g_LuaState;

    void RegisterTypesAndFunctionsToLua( sol::state& lua )
    {
        lua.open_libraries( sol::lib::base, sol::lib::math );
        RegisterLuaFunctions_Math( lua );
        RegisterLuaFunctions_Input( lua );
        RegisterLuaFunctions_ECS( lua ); // This must come before registering any components with the ECS
        auto luaTimeNamespace = lua["Time"].get_or_create< sol::table >();
        luaTimeNamespace["dt"] = 0;

        sol::usertype< ScriptComponent > scriptComponent_type = lua.new_usertype< ScriptComponent >( "ScriptComponent" );
        scriptComponent_type["GetFunction"] = &ScriptComponent::GetFunction;
        REGISTER_COMPONENT_WITH_ECS( lua, ScriptComponent,
            static_cast< ScriptComponent&( entt::registry::* )( const entt::entity ) >( &entt::registry::assign< ScriptComponent > ) );

        sol::usertype< NameComponent > nameComponent_type = lua.new_usertype< NameComponent >( "NameComponent" );
        nameComponent_type["name"] = &NameComponent::name;
        REGISTER_COMPONENT_WITH_ECS( lua, NameComponent,
            static_cast< NameComponent&( entt::registry::* )( const entt::entity )> ( &entt::registry::assign< NameComponent > ) );

        sol::usertype< Transform > transform_type = lua.new_usertype< Transform >( "Transform" );
        transform_type["position"] = &Transform::position;
        transform_type["rotation"] = &Transform::rotation;
        transform_type["scale"] = &Transform::scale;
        transform_type["GetModelMatrix"] = &Transform::GetModelMatrix;
        REGISTER_COMPONENT_WITH_ECS( lua, Transform,
            static_cast< Transform&( entt::registry::* )( const entt::entity )> ( &entt::registry::assign< Transform > ) );

        sol::usertype< Camera > camera_type = lua.new_usertype< Camera >( "Camera", sol::constructors< Camera() >() );
        camera_type["position"]                 = &Camera::position;
        camera_type["rotation"]                 = &Camera::rotation;
        camera_type["fov"]                      = &Camera::fov;
        camera_type["aspectRatio"]              = &Camera::aspectRatio;
        camera_type["nearPlane"]                = &Camera::nearPlane;
        camera_type["farPlane"]                 = &Camera::farPlane;
        camera_type["UpdateFrustum"]            = &Camera::UpdateFrustum;
        camera_type["UpdateOrientationVectors"] = &Camera::UpdateOrientationVectors;
        camera_type["UpdateViewMatrix"]         = &Camera::UpdateViewMatrix;
        camera_type["UpdateProjectionMatrix"]   = &Camera::UpdateProjectionMatrix;
        camera_type["GetV"]                     = &Camera::GetV;
        camera_type["GetP"]                     = &Camera::GetP;
        camera_type["GetVP"]                    = &Camera::GetVP;
        camera_type["GetForwardDir"]            = &Camera::GetForwardDir;
        camera_type["GetUpDir"]                 = &Camera::GetUpDir;
        camera_type["GetRightDir"]              = &Camera::GetRightDir;

        sol::usertype< Scene > scene_type = lua.new_usertype< Scene >( "Scene" );
        scene_type["camera"] = &Scene::camera;
    }

} // namespace Progression
