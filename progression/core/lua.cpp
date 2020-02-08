#include "core/lua.hpp"
#include "core/camera.hpp"
#include "core/ecs.hpp"
#include "core/input.hpp"
#include "core/math.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api/ui.hpp"
#include "resource/resource_manager.hpp"

namespace Progression
{

    sol::state* g_LuaState;

    void RegisterTypesAndFunctionsToLua( sol::state& lua )
    {
        lua.open_libraries( sol::lib::base, sol::lib::math );
        RegisterLuaFunctions_Math( lua );
        RegisterLuaFunctions_Input( lua );
        RegisterLuaFunctions_Window( lua );
        RegisterLuaFunctions_ECS( lua ); // This must come before registering any components with the ECS
        RegisterLuaFunctions_Resource( lua );

        auto luaTimeNamespace = lua["Time"].get_or_create< sol::table >();
        luaTimeNamespace["dt"] = 0;

        auto luaUINamespace = lua["UI"].get_or_create< sol::table >();
        luaUINamespace["CapturingMouse"] = &Gfx::UIOverlay::CapturingMouse;
        luaUINamespace["Visible"]        = &Gfx::UIOverlay::Visible;
        luaUINamespace["SetVisible"]     = &Gfx::UIOverlay::SetVisible;

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

        sol::usertype< DirectionalLight > directional_light_type = lua.new_usertype< DirectionalLight >( "DirectionalLight" );
        directional_light_type["colorAndIntensity"] = &DirectionalLight::colorAndIntensity;
        directional_light_type["direction"] = &DirectionalLight::direction;

        sol::usertype< Scene > scene_type = lua.new_usertype< Scene >( "Scene" );
        scene_type["camera"]           = &Scene::camera;
        scene_type["directionalLight"] = &Scene::directionalLight;
    }

} // namespace Progression
