#include "components/factory.hpp"
#include "components/transform.hpp"
#include "components/entity_metadata.hpp"
#include "utils/json_parsing.hpp"
#include "core/assert.hpp"
#include "utils/fileIO.hpp"
#include <unordered_map>

namespace Progression
{

    static void ParseTransform( rapidjson::Value& value, const entt::entity e, entt::registry& registry )
    {
        Transform& t = registry.assign< Transform >( e );
        static FunctionMapper< void, Transform& > mapping(
        {
            { "position", []( rapidjson::Value& v, Transform& t ) { t.position = ParseVec3( v ); } },
            { "rotation", []( rapidjson::Value& v, Transform& t ) { t.rotation = ParseVec3( v ); } },
            { "scale",    []( rapidjson::Value& v, Transform& t ) { t.scale    = ParseVec3( v ); } },
        });

        mapping.ForEachMember( value, t );
    }

    static void ParseEntityMetaData( rapidjson::Value& value, const entt::entity e, entt::registry& registry )
    {
        PG_ASSERT( value.IsObject() );
        EntityMetaData& d = registry.assign< EntityMetaData >( e );
        static FunctionMapper< void, EntityMetaData& > mapping(
        {
            { "parent", [&registry]( rapidjson::Value& v, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsString() );
                    std::string parentName = v.GetString();
                    d.parent = GetEntityByName( registry, parentName );
                    PG_ASSERT( d.parent != entt::null, "No entity found with name '" + parentName + "'" );
                }
            },
            { "isStatic", []( rapidjson::Value& v, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsBool() );
                    d.isStatic = v.GetBool();
                }
            }
        });

        mapping.ForEachMember( value, d );
    }

    static void ParseNameComponent( rapidjson::Value& value, const entt::entity e, entt::registry& registry )
    {
        PG_ASSERT( value.IsString() );
        NameComponent& comp = registry.assign< NameComponent >( e );
        comp.name = value.GetString();
    }

    void ParseComponent( rapidjson::Value& value, const entt::entity e, entt::registry& registry, const std::string& typeName )
    {
        static FunctionMapper< void, const entt::entity, entt::registry& > mapping(
        {
            { "EntityMetaData", ParseEntityMetaData },
            { "name",           ParseNameComponent },
            { "Transform",      ParseTransform },
        });

        mapping[typeName]( value, e, registry );
    }

} // namespace Progression
