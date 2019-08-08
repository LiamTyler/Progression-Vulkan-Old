#include "core/scene.hpp"
#include "core/ecs.hpp"
#include "graphics/lights.hpp"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <sstream>

#include "components/model_renderer_component.hpp"

using namespace Progression;

static bool ParseResourcefile( std::istream& in )
{
    std::string fname;
    fileIO::parseLineKeyVal( in, "filename", fname );
    return !in.fail() && ResourceManager::LoadFastFile( PG_RESOURCE_DIR + fname );
}

static bool ParseCamera( Scene* scene, std::istream& in )
{
    Camera& camera = scene->camera;
    std::string line;
    float tmp;

    fileIO::parseLineKeyVal( in, "position", camera.position );
    glm::vec3 rotInDegrees;
    fileIO::parseLineKeyVal( in, "rotation", rotInDegrees );
    camera.rotation = glm::radians( rotInDegrees );
    fileIO::parseLineKeyVal( in, "fov", tmp );
    camera.SetFOV( glm::radians( tmp ) );
    std::getline( in, line );
    std::stringstream ss( line );
    std::string first;
    ss >> first;
    PG_ASSERT( first == "aspect" );
    float width, height;
    char colon;
    ss >> width >> colon >> height;
    camera.SetAspectRatio( width / height );
    fileIO::parseLineKeyVal( in, "near-plane", tmp );
    camera.SetNearPlane( tmp );
    fileIO::parseLineKeyVal( in, "far-plane", tmp );
    camera.SetFarPlane( tmp );

    camera.UpdateOrientationVectors();
    camera.UpdateViewMatrix();

    return !in.fail();
}

static bool ParseLight( Scene* scene, std::istream& in )
{
    Light light;
    std::string tmp;

    fileIO::parseLineKeyVal( in, "name", light.name );
    fileIO::parseLineKeyVal( in, "type", tmp );
    PG_ASSERT( tmp == "point" || tmp == "spot" || tmp == "directional" );
    light.type = tmp == "point" ? Light::Type::POINT
                                : tmp == "spot" ? Light::Type::SPOT : Light::Type::DIRECTIONAL;
    light.enabled = fileIO::parseLineKeyBool( in, "enabled" );
    fileIO::parseLineKeyVal( in, "color", light.color );
    fileIO::parseLineKeyVal( in, "intensity", light.intensity );
    fileIO::parseLineKeyValOptional( in, "position", light.position );
    fileIO::parseLineKeyValOptional( in, "direction", light.direction );
    light.direction = glm::normalize( light.direction );
    fileIO::parseLineKeyValOptional( in, "radius", light.radius );
    fileIO::parseLineKeyValOptional( in, "innerCutoff", light.innerCutoff );
    fileIO::parseLineKeyValOptional( in, "outerCutoff", light.outerCutoff );

    scene->AddLight( new Light( light ) );
    return !in.fail();
}

static bool ParseEntity( std::istream& in )
{
    auto e                = ECS::entity::create();
    ECS::EntityData* data = ECS::entity::data( e );
    fileIO::parseLineKeyValOptional( in, "name", data->name );
    std::string parentName;
    fileIO::parseLineKeyValOptional( in, "parent", parentName );
    if ( parentName.empty() )
        data->parentID = ECS::INVALID_ENTITY_ID;
    fileIO::parseLineKeyVal( in, "position", data->transform.position );
    fileIO::parseLineKeyVal( in, "rotation", data->transform.rotation );
    data->transform.rotation = glm::radians( data->transform.rotation );
    fileIO::parseLineKeyVal( in, "scale", data->transform.scale );
    data->isStatic = fileIO::parseLineKeyBool( in, "static" );
    int numComponents;
    fileIO::parseLineKeyVal( in, "numComponents", numComponents );
    for ( int i = 0; i < numComponents; ++i )
    {
        std::string component;
        fileIO::parseLineKeyVal( in, "Component", component );
        if ( component == "ModelRenderer" )
        {
            auto comp = ECS::component::create< ModelRenderComponent >( e );
            ParseModelRendererComponentFromFile( in, *comp );
        }
        else
        {
            LOG_WARN( "Unrecognized component: '", component, "'" );
        }
    }

    return !in.fail();
}

static bool ParseBackgroundColor( Scene* scene, std::istream& in )
{
    fileIO::parseLineKeyVal( in, "color", scene->backgroundColor );

    return !in.fail();
}

static bool ParseAmbientColor( Scene* scene, std::istream& in )
{
    fileIO::parseLineKeyVal( in, "color", scene->ambientColor );

    return !in.fail();
}

namespace Progression
{

Scene::~Scene()
{
    for ( auto light : m_lights )
    {
        delete light;
    }
}

void Scene::AddLight( Light* light )
{
    switch ( light->type )
    {
        case Light::Type::POINT:
            ++m_numPointLights;
            break;
        case Light::Type::SPOT:
            ++m_numSpotLights;
            break;
        case Light::Type::DIRECTIONAL:
            ++m_numDirectionalLights;
            break;
    }
    m_lights.push_back( light );
}

void Scene::RemoveLight( Light* light )
{
    m_lights.erase( std::remove( m_lights.begin(), m_lights.end(), light ), m_lights.end() );
}

void Scene::SortLights()
{
    std::sort( m_lights.begin(), m_lights.end(),
               []( Light* lhs, Light* rhs ) { return lhs->type < rhs->type; } );
}

#define PARSE_SCENE_ELEMENT( name, function )                       \
    else if ( line == name )                                        \
    {                                                               \
        if ( !function )                                            \
        {                                                           \
            LOG_ERR( "Could not parse '" #name "' in scene file" ); \
            delete scene;                                           \
            return nullptr;                                         \
        }                                                           \
    }

Scene* Scene::Load( const std::string& filename )
{
    Scene* scene = new Scene;

    std::ifstream in( filename );
    if ( !in )
    {
        LOG_ERR( "Could not open scene:", filename );
        return nullptr;
    }

    std::string line;
    while ( std::getline( in, line ) )
    {
        if ( line == "" || line[0] == '#' )
        {
            continue;
        }
        PARSE_SCENE_ELEMENT( "Resourcefile", ParseResourcefile( in ) )
        PARSE_SCENE_ELEMENT( "Camera", ParseCamera( scene, in ) )
        PARSE_SCENE_ELEMENT( "Light", ParseLight( scene, in ) )
        PARSE_SCENE_ELEMENT( "Entity", ParseEntity( in ) )
        PARSE_SCENE_ELEMENT( "BackgroundColor", ParseBackgroundColor( scene, in ) )
        PARSE_SCENE_ELEMENT( "AmbientColor", ParseAmbientColor( scene, in ) )
        else
        {
            LOG_WARN( "Unrecognized scene file entry: ", line );
        }
    }

    in.close();

    return scene;
}

const std::vector<Light*>& Scene::GetLights() const
{
    return m_lights;
}

unsigned int Scene::GetNumPointLights() const
{
    return m_numPointLights;
}

unsigned int Scene::GetNumSpotLights() const
{
    return m_numSpotLights;
}

unsigned int Scene::GetNumDirectionalLights() const
{
    return m_numDirectionalLights;
}

} // namespace Progression
