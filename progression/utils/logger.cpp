#include "utils/logger.hpp"
#include "core/unused.hpp"

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
    m_useColors = useColors;

    if ( filename == "" )
    {
        return;
    }

    m_outputFile.open( filename );
    if ( !m_outputFile )
    {
        LOG_ERR( "Failed to open log file: '" + filename + "'" );
    }
#endif // #if !USING( SHIP_BUILD )
}

void Logger::Shutdown()
{
#if !USING( SHIP_BUILD )
    if ( m_outputFile )
    {
        m_outputFile.close();
    }
#endif // #if !USING( SHIP_BUILD )
}

std::ostream& Logger::OutputLocation()
{
    if ( m_outputFile )
    {
        return m_outputFile;
    }
    else
    {
        return std::cout;
    }
}
