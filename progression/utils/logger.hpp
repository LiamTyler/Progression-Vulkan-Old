#pragma once

#include "core/platform_defines.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#if USING( SHIP_BUILD )

#define LOG( ... ) do {} while(0)
#define LOG_WARN( ... ) do {} while(0)
#define LOG_ERR( ... ) do {} while(0)

#else // #if USING( SHIP_BUILD )

#define LOG( ... ) g_Logger.Write( Logger::DEBUG, __VA_ARGS__ )
#define LOG_WARN( ... ) g_Logger.Write( Logger::WARN, __VA_ARGS__ )
#define LOG_ERR( ... ) g_Logger.Write( Logger::ERR, __VA_ARGS__ )

#endif // #else // #if USING( SHIP_BUILD )

class PrintModifier
{
public:
    enum Color
    {
        RED     = 31,
        GREEN   = 32,
        YELLOW  = 33,
        BLUE    = 34,
        DEFAULT = 39
    };

    enum Emphasis
    {
        NONE      = 0,
        BOLD      = 1,
        UNDERLINE = 4
    };

    PrintModifier( Color c = DEFAULT, Emphasis e = NONE );

    friend std::ostream& operator<<( std::ostream& out, const PrintModifier& mod );

    Color color;
    Emphasis emphasis;
};

class Logger
{
public:
    enum Severity
    {
        DEBUG,
        WARN,
        ERR
    };

    Logger() = default;

    void Init( const std::string& filename = "", bool useColors = true );
    void Shutdown();

    template < typename T >
    void PrintArgs( std::ostream& out, T t )
    {
        out << t;
    }

    template < typename T, typename U, typename... Args >
    void PrintArgs( std::ostream& out, T t, U u, Args... args )
    {
        out << t;
        PrintArgs( out, u, args... );
    }

    template < typename... Args >
    void Write( Severity sev, Args&&... args )
    {
        std::string severity;
        PrintModifier mod;
        switch ( sev )
        {
            case DEBUG:
                severity = "";
                mod      = PrintModifier( PrintModifier::GREEN, PrintModifier::NONE );
                break;
            case WARN:
                severity = "WARNING  ";
                mod      = PrintModifier( PrintModifier::YELLOW, PrintModifier::NONE );
                break;
            case ERR:
                severity = "ERROR    ";
                mod      = PrintModifier( PrintModifier::RED, PrintModifier::NONE );
                break;
        }

        m_lock.lock();
        if ( m_outputFile.is_open() )
        {
            PrintArgs( m_outputFile, severity, args... );
            m_outputFile << std::endl;
        }
        else
        {
            if ( m_useColors )
            {
                std::cout << mod << severity;
                PrintArgs( std::cout, args... );
                std::cout << PrintModifier() << std::endl;
            }
            else
            {
                PrintArgs( std::cout, severity, std::forward< Args >( args )... );
                std::cout << std::endl;
            }
        }
        m_lock.unlock();
    }

    void UseColors( bool b ) { m_useColors = b; }

private:
    bool m_useColors;
    std::ofstream m_outputFile;
    std::mutex m_lock;
};

extern Logger g_Logger;
