#include "components/factory.hpp"
#include "components/transform.hpp"
#include "components/entity_metadata.hpp"
#include "components/model_renderer.hpp"
#include "components/script_component.hpp"
#include "resource/model.hpp"
#include "resource/material.hpp"
#include "core/assert.hpp"
#include "resource/resource_manager.hpp"
#include "utils/json_parsing.hpp"
#include "core/assert.hpp"
#include "utils/fileIO.hpp"
#include <unordered_map>

namespace Progression
{

    static void ParseEntityMetaData( rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        PG_ASSERT( value.IsObject() );
        EntityMetaData& d = registry.assign< EntityMetaData >( e );
        static FunctionMapper< void, entt::registry&, EntityMetaData& > mapping(
        {
            { "parent", []( rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsString() );
                    std::string parentName = v.GetString();
                    d.parent = GetEntityByName( reg, parentName );
                    PG_ASSERT( d.parent != entt::null, "No entity found with name '" + parentName + "'" );
                }
            },
            { "isStatic", []( rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsBool() );
                    d.isStatic = v.GetBool();
                }
            }
        });

        mapping.ForEachMember( value, registry, d );
    }

    static void ParseNameComponent( rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        PG_ASSERT( value.IsString() );
        NameComponent& comp = registry.assign< NameComponent >( e );
        comp.name = value.GetString();
    }

    static void ParseScriptComponent( rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        ScriptComponent& s = registry.assign< ScriptComponent >( e );
        static FunctionMapper< void, ScriptComponent& > mapping(
        {
            { "script", []( rapidjson::Value& v, ScriptComponent& s )
                {
                    PG_ASSERT( v.IsString(), "Please provide a string with the script name" );
                    auto ptr = ResourceManager::Get< Script >( v.GetString() );
                    if ( !ptr )
                    {
                        LOG_ERR( "Could not find script with name '", v.GetString(), "'" );
                    }
                    else
                    {
                        s.AddScript( ptr );
                    }
                }
            },
        });

        mapping.ForEachMember( value, s );
    }

    static void ParseTransform( rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        Transform& t = registry.assign< Transform >( e );
        static FunctionMapper< void, Transform& > mapping(
        {
            { "position", []( rapidjson::Value& v, Transform& t ) { t.position = ParseVec3( v ); } },
            { "rotation", []( rapidjson::Value& v, Transform& t ) { t.rotation = glm::radians( ParseVec3( v ) ); } },
            { "scale",    []( rapidjson::Value& v, Transform& t ) { t.scale    = ParseVec3( v ); } },
        });

        mapping.ForEachMember( value, t );
    }

    static void ParseModelRenderer( rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {   
        auto& comp = registry.assign< ModelRenderer >( e );
        static FunctionMapper< void, ModelRenderer& > mapping(
        {
            { "model", []( rapidjson::Value& v, ModelRenderer& comp )
                {
                    PG_ASSERT( v.IsString(), "Please provide a string of the model's name" );
                    comp.model = ResourceManager::Get< Model >( v.GetString() );
                    PG_ASSERT( comp.model != nullptr, "Model with name '" + std::string( v.GetString() ) + "' not found" );
                    comp.materials = comp.model->materials;
                }
            },
            { "material", []( rapidjson::Value& v, ModelRenderer& comp )
                {
                    PG_ASSERT( v.IsString(), "Please provide a string of the material's name" );
                    auto mat = ResourceManager::Get< Material >( v.GetString() );
                    PG_ASSERT( mat != nullptr, "Material with name '" + std::string( v.GetString() ) + "' not found" );
                    PG_ASSERT( comp.model != nullptr, "Must specify model before assigning materials for it" );
                    for ( auto& matPtr : comp.materials )
                    {
                        matPtr = mat;
                    }
                }
            },
        });

        mapping.ForEachMember( value, comp );
    }

    void ParseComponent( rapidjson::Value& value, entt::entity e, entt::registry& registry, const std::string& typeName )
    {
        static FunctionMapper< void, entt::entity, entt::registry& > mapping(
        {
            { "EntityMetaData",  ParseEntityMetaData },
            { "NameComponent",   ParseNameComponent },
            { "ScriptComponent", ParseScriptComponent },
            { "Transform",       ParseTransform },
            { "ModelRenderer",   ParseModelRenderer },
        });

        //mapping[typeName]( value, e, registry );
        mapping.Evaluate( typeName, value, std::move( e ), registry );
    }

} // namespace Progression
