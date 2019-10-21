#include "resource/script.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include <fstream>

namespace Progression
{

bool Script::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    ScriptCreateInfo* info = static_cast< ScriptCreateInfo* >( createInfo );
    name       = info->name;
    // std::ifstream in( info->filename, std::ios::binary );
    std::ifstream in( info->filename );
    if ( !in )
    {
        LOG_ERR( "Could not open script file '", info->filename, "'" );
        return false;
    }

    in.seekg( 0, std::ios::end );
    size_t size = in.tellg();
    scriptText.resize( size );
    in.seekg( 0 );
    in.read( &scriptText[0], size ); 

    return true;
}

void Script::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Script >( dst ) );
    Script* dstPtr = (Script*) dst.get();
    *dstPtr        = std::move( *this );
}

bool Script::Serialize( std::ofstream& out ) const
{
    serialize::Write( out, scriptText );

    return !out.fail();
}

bool Script::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
    serialize::Read( buffer, scriptText );

    return true;
}

} // namespace Progression
