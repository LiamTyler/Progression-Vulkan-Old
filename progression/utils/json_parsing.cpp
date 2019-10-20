#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include <vector>

rapidjson::Document ParseJSONFile( const std::string& filename )
{
    FILE* fp = fopen( filename.c_str(), "r" );
    if ( fp == NULL )
    {
        LOG_ERR( "Could not open json file '", filename, "'" );
        return {};
    }
    fseek( fp, 0L, SEEK_END );
    auto fileSize = ftell( fp ) + 1;
    fseek( fp, 0L, SEEK_SET );
    std::vector< char > buffer( fileSize );
    rapidjson::FileReadStream is( fp, buffer.data(), fileSize );

    rapidjson::Document d;
    rapidjson::ParseResult ok = d.ParseStream( is );
    if ( !ok )
    {
        LOG_ERR( "JSON parse error: ", rapidjson::GetParseError_En( ok.Code()), " (", ok.Offset(), ")" );
    }

    fclose( fp );

    return d;
}

glm::vec3 ParseVec3( rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 3 );
    auto& GetF = ParseNumber< float >;
    return glm::vec3( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ) );
}

glm::vec4 ParseVec4( rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 4 );
    auto& GetF = ParseNumber< float >;
    return glm::vec4( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ), GetF( v[3] ) );
}
