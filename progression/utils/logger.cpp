#include "utils/logger.hpp"

Logger g_Logger;

PrintModifier::PrintModifier( Color c, Emphasis e ) :
    color(c),
    emphasis(e)
{
}

std::ostream& operator<<( std::ostream& out, const PrintModifier& mod )
{
    return out << "\033[" << mod.emphasis << ";" << mod.color << "m";
}

void Logger::Init( const std::string& filename, bool useColors )
{
    m_useColors = useColors;

    if (filename == "")
        return;

    m_outputFile.open( filename );
    if ( !m_outputFile )
    {
        LOG_ERR( "Failed to open log file: '" + filename + "'" );
    }
}

void Logger::Shutdown()
{
    if ( m_outputFile )
    {
        m_outputFile.close();
    }
}