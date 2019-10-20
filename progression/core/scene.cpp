#include "core/scene.hpp"
#include "core/assert.hpp"
#include "components/factory.hpp"
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
        { "rotation",    []( rapidjson::Value& v, Camera& camera ) { camera.rotation    = ParseVec3( v ); } },
        { "fov",         []( rapidjson::Value& v, Camera& camera ) { camera.fov         = glm::radians( ParseNumber< float >( v ) ); } },
        { "aspectRatio", []( rapidjson::Value& v, Camera& camera ) { camera.aspectRatio = ParseNumber< float >( v ); } },
        { "nearPlane",   []( rapidjson::Value& v, Camera& camera ) { camera.nearPlane   = ParseNumber< float >( v ); } },
        { "farPlane",    []( rapidjson::Value& v, Camera& camera ) { camera.farPlane    = ParseNumber< float >( v ); } },
    });

    mapping.ForEachMember( v, camera );

    camera.UpdateFrustum();
    camera.UpdateProjectionMatrix();
}

static void ParseLight( rapidjson::Value& value, Scene* scene )
{
    SpotLight light;
    std::string type;
    static FunctionMapper< void, SpotLight& > mapping(
    {
        { "colorAndIntensity", []( rapidjson::Value& v, SpotLight& l ) { l.colorAndIntensity = ParseVec4( v ); } },
        { "type",             [&]( rapidjson::Value& v, SpotLight& l ) { type = v.GetString(); } },
        { "direction",         []( rapidjson::Value& v, SpotLight& l ) { l.direction = glm::normalize( ParseVec3( v ) ); } },
        { "position",          []( rapidjson::Value& v, SpotLight& l ) { l.position = ParseVec3( v ); } },
        { "radius",            []( rapidjson::Value& v, SpotLight& l ) { l.radius = ParseNumber< float >( v ); } },
        { "cutoff",            []( rapidjson::Value& v, SpotLight& l ) { l.cutoff = glm::radians( ParseNumber< float >( v ) ); } }
    });
    mapping.ForEachMember( value, light );

    PG_ASSERT( type == "Directional" || type == "Point" || type == "Spot" );
    if ( type == "Directional" )
    {
        scene->directionalLight.colorAndIntensity = light.colorAndIntensity;
        scene->directionalLight.direction         = light.direction;
    }
    else if ( type == "Point" )
    {
        PointLight l;
        l.colorAndIntensity = light.colorAndIntensity;
        l.position          = light.position;
        l.radius            = light.radius;
        scene->pointLights.push_back( l );
    }
    else
    {
        scene->spotLights.push_back( light );
    }
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

    auto document = ParseJSONFile( filename );
    if ( document.IsNull() )
    {
        delete scene;
        return nullptr;
    }

    static FunctionMapper< void, Scene* > mapping(
    {
        { "AmbientColor",    ParseAmbientColor },
        { "BackgroundColor", ParseBackgroundColor },
        { "Camera",          ParseCamera },
        { "Entity",          ParseEntity },
        { "Light",           ParseLight },
        { "Resourcefile",    ParseResourcefile },
    });

    mapping.ForEachMember( document, std::move( scene ) );

    return scene;
}

} // namespace Progression
