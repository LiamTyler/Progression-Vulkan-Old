#include "utils/logger.hpp"
#include "core/unused.hpp"
#include <filesystem>

Logger g_Logger;

PrintModifier::PrintModifier( Color c, Emphasis e ) :
    color( c ),
    emphasis( e )
{
}

std::ostream& operator<<( std::ostream& out, const PrintModifier& mod )
{
    return out << "\033[" << mod.emphasis << ";" << mod.color << "m";
}

void Logger::Init( const std::string& filename, bool useColors )
{
    PG_MAYBE_UNUSED( filename );
    PG_MAYBE_UNUSED( useColors );

#if !USING( SHIP_BUILD )
    std::filesystem::create_directory( PG_ROOT_DIR "logs" );
    AddLocation( "stdout", &std::cout, useColors );
    if ( filename != "" )
    {
        AddLocation( "configFileOutput", filename );
    }
    
#endif // #if !USING( SHIP_BUILD )
}

void Logger::Shutdown()
{
#if !USING( SHIP_BUILD )
    outputs.clear();
#endif // #if !USING( SHIP_BUILD )
}
