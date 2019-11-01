#include "core/scene.hpp"
#include "core/assert.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "components/factory.hpp"
#include "components/script_component.hpp"
#include "graphics/lights.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace Progression;

static void ParseResourcefile( rapidjson::Value& v, Scene* scene )
{
    PG_UNUSED( scene );
    PG_ASSERT( v.HasMember( "filename" ) );
    auto& member = v["filename"];
    PG_ASSERT( member.IsString() );
    std::string fname = member.GetString();
    ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/" + fname );
}

static void ParseCamera( rapidjson::Value& v, Scene* scene )
{
    Camera& camera = scene->camera;
    static FunctionMapper< void, Camera& > mapping(
    {
        { "position",    []( rapidjson::Value& v, Camera& camera ) { camera.position    = ParseVec3( v ); } },
        { "rotation",    []( rapidjson::Value& v, Camera& camera ) { camera.rotation    = glm::radians( ParseVec3( v ) ); } },
        { "fov",         []( rapidjson::Value& v, Camera& camera ) { camera.fov         = glm::radians( ParseNumber< float >( v ) ); } },
        { "aspectRatio", []( rapidjson::Value& v, Camera& camera ) { camera.aspectRatio = ParseNumber< float >( v ); } },
        { "nearPlane",   []( rapidjson::Value& v, Camera& camera ) { camera.nearPlane   = ParseNumber< float >( v ); } },
        { "farPlane",    []( rapidjson::Value& v, Camera& camera ) { camera.farPlane    = ParseNumber< float >( v ); } },
    });

    mapping.ForEachMember( v, camera );

    camera.UpdateFrustum();
    camera.UpdateProjectionMatrix();
}

static void ParseDirectionalLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, DirectionalLight& > mapping(
    {
        { "colorAndIntensity", []( rapidjson::Value& v, DirectionalLight& l ) { l.colorAndIntensity = ParseVec4( v ); } },
        { "direction",         []( rapidjson::Value& v, DirectionalLight& l )
            { l.direction = glm::vec4( glm::normalize( ParseVec3( v ) ), 0 ); }
        }
    });
    scene->directionalLight.colorAndIntensity = glm::vec4( 1 );
    scene->directionalLight.direction         = glm::vec4( 0, 0, -1, 0 );
    mapping.ForEachMember( value, scene->directionalLight );
}

static void ParsePointLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, PointLight& > mapping(
    {
        { "colorAndIntensity", []( rapidjson::Value& v, PointLight& l ) { l.colorAndIntensity = ParseVec4( v ); } },
        { "positionAndRadius", []( rapidjson::Value& v, PointLight& l ) { l.positionAndRadius = ParseVec4( v ); } },
    });
    PointLight p;
    p.colorAndIntensity = glm::vec4( 1 );
    p.positionAndRadius = glm::vec4( 0, 0, 0, 10 );
    scene->pointLights.emplace_back( p );
    mapping.ForEachMember( value, scene->pointLights[scene->pointLights.size() - 1] );
}

static void ParseSpotLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, SpotLight& > mapping(
    {
        { "colorAndIntensity",  []( rapidjson::Value& v, SpotLight& l ) { l.colorAndIntensity = ParseVec4( v ); } },
        { "positionAndRadius",  []( rapidjson::Value& v, SpotLight& l ) { l.positionAndRadius = ParseVec4( v ); } },
        { "directionAndCutoff", []( rapidjson::Value& v, SpotLight& l )
            {
                l.directionAndCutoff = ParseVec4( v );
                glm::vec3 d          = glm::normalize( glm::vec3( l.directionAndCutoff ) );
                l.directionAndCutoff = glm::vec4( d, glm::radians( l.directionAndCutoff.w ) );
            }
        },
    });
    SpotLight p;
    p.colorAndIntensity  = glm::vec4( 1 );
    p.positionAndRadius  = glm::vec4( 0, 0, 0, 10 );
    p.directionAndCutoff = glm::vec4( 0, 0, -1, glm::radians( 20.0f ) );
    scene->spotLights.emplace_back( p );
    mapping.ForEachMember( value, scene->spotLights[scene->spotLights.size() - 1] );
}

static void ParseEntity( rapidjson::Value& v, Scene* scene )
{
    auto e = scene->registry.create();
    for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
    {
        ParseComponent( it->value, e, scene->registry, it->name.GetString() );
    }
}

static void ParseBackgroundColor( rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.HasMember( "color" ) );
    auto& member           = v["color"];
    scene->backgroundColor = ParseVec3( member );
}

static void ParseAmbientColor( rapidjson::Value& v, Scene* scene )
{
    PG_ASSERT( v.HasMember( "color" ) );
    auto& member        = v["color"];
    scene->ambientColor = ParseVec3( member );
}

namespace Progression
{

Scene* Scene::Load( const std::string& filename )
{
    Scene* scene = new Scene;
    scene->directionalLight.colorAndIntensity.w = 0; // turn off the directional light to start

    auto document = ParseJSONFile( filename );
    if ( document.IsNull() )
    {
        delete scene;
        return nullptr;
    }

    static FunctionMapper< void, Scene* > mapping(
    {
        { "AmbientColor",     ParseAmbientColor },
        { "BackgroundColor",  ParseBackgroundColor },
        { "Camera",           ParseCamera },
        { "Entity",           ParseEntity },
        { "DirectionalLight", ParseDirectionalLight },
        { "PointLight",       ParsePointLight },
        { "SpotLight",        ParseSpotLight },
        { "Resourcefile",     ParseResourcefile },
    });

    mapping.ForEachMember( document, std::move( scene ) );

    return scene;
}

void Scene::Start()
{
    g_LuaState["registry"] = &registry;
    g_LuaState["scene"] = this;
    registry.view< ScriptComponent >().each([]( const entt::entity e, ScriptComponent& comp )
    {
        for ( int i = 0; i < comp.numScripts; ++i )
        {
            auto startFn = comp.scripts[i].env["Start"];
            if ( startFn.valid() )
            {
                comp.scripts[i].env["entity"] = e;
                CHECK_SOL_FUNCTION_CALL( startFn() );
            }
        }
    });
}

void Scene::Update()
{
    auto luaTimeNamespace = g_LuaState["Time"].get_or_create< sol::table >();
    luaTimeNamespace["dt"] = Time::DeltaTime();
    g_LuaState["registry"] = &registry;
    registry.view< ScriptComponent >().each([]( const entt::entity e, ScriptComponent& comp )
    {
        for ( int i = 0; i < comp.numScriptsWithUpdate; ++i )
        {
            comp.scripts[i].env["entity"] = e;
            CHECK_SOL_FUNCTION_CALL( comp.scripts[i].updateFunc.second() );
        }
    });
}

} // namespace Progression
