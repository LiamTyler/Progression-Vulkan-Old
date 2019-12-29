#pragma once

#include "core/platform_defines.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

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

class LoggerOutputLocation
{
public:
    LoggerOutputLocation( const std::string& _name, std::ostream* out, bool useColors = true, bool _isFile = false ) :
        name( _name ),
        output( out ),
        colored( useColors ),
        isFile( _isFile )
    {
    }

    LoggerOutputLocation( const std::string& _name, const std::string& filename ) :
        name( _name ),
        output( new std::ofstream( filename ) ),
        colored( false ),
        isFile( true )
    {
    }

    ~LoggerOutputLocation()
    {
        if ( isFile && output )
        {
            delete output;
        }
    }

    std::string name;
    std::ostream* output;
    bool colored;
    bool isFile;
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

    void AddLocation( const std::string& name, std::ostream* output, bool useColors = true, bool isFile = false )
    {
        outputs.emplace_back( name, output, useColors, isFile );
    }

    void AddLocation( const std::string& name, const std::string& filename )
    {
        outputs.emplace_back( name, filename );
        if ( !outputs[outputs.size() - 1].output || !( ( std::ofstream*) outputs[outputs.size() - 1].output )->is_open() )
        {
            outputs.pop_back();
            std::cout << "Failed to open logging location '" << filename << "'" << std::endl;
        }
    }

    LoggerOutputLocation* GetLocation( const std::string& name )
    {
        for ( auto& output : outputs )
        {
            if ( output.name == name )
            {
                return &output;
            }
        }

        return nullptr;
    }

    void RemoveLocation( const std::string& name )
    {
        size_t index = -1;
        for ( size_t i = 0; i < outputs.size(); ++i )
        {
            if ( name == outputs[i].name )
            {
                index = i;
                break;
            }
        }
        if ( index != -1 )
        {
            outputs.erase( outputs.begin() + index );
        }
    }

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
        for ( const auto& output : outputs )
        {
            if ( output.colored )
            {
                std::cout << mod << severity;
                PrintArgs( *output.output, std::forward< Args >( args )... );
                *output.output << "\033[0m" << std::endl;
            }
            else
            {
                PrintArgs( *output.output, severity, std::forward< Args >( args )... );
                *output.output << std::endl;
            }
        }
        m_lock.unlock();
    }

private:
    std::vector< LoggerOutputLocation > outputs;
    std::mutex m_lock;
};

extern Logger g_Logger;
