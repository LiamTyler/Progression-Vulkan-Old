#include "core/assert.hpp"
#include "core/core_defines.hpp"
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace fileIO {

    template < typename T >
    void Parse( std::istream& in, T& val )
    {
        in >> val;
        PG_ASSERT( !in.fail() );
    }

    inline void Parse( std::istream& in, bool& val )
    {
        in >> std::boolalpha >> val;
        PG_ASSERT( !in.fail() );
    }

    template < typename T >
    void ParseLineKeyVal( std::istream& in, const char* key, T& val )
    {
        PG_MAYBE_UNUSED( key );
        std::string line;
        std::string s;
        std::getline( in, line );
        auto ss = std::istringstream( line );
        ss >> s;
        PG_ASSERT( s == key, ( "Expecting key '" + std::string( key ) + "' while parsing file" ).c_str() );
        ss >> val;
        PG_ASSERT( !in.fail() && !ss.fail() );
    }

    inline void ParseLineKeyVal( std::istream& in, const char* key, bool& val )
    {
        PG_MAYBE_UNUSED( key );
        std::string line;
        std::string s;
        std::getline( in, line );
        auto ss = std::istringstream( line );
        ss >> s;
        PG_ASSERT( s == key, ( "Expecting key '" + std::string( key ) + "' while parsing file" ).c_str() );
        ss >> std::boolalpha >> val;
        PG_ASSERT( !in.fail() && !ss.fail() );
    }

    template <typename T>
    bool ParseLineKeyValOptional( std::istream& in, const char* key, T& val )
    {
        PG_MAYBE_UNUSED( key );
        std::string line;
        std::string s;
        std::getline( in, line );
        auto ss = std::istringstream( line );
        ss >> s;
        PG_ASSERT( s == key, ( "Expecting key '" + std::string( key ) + "' while parsing file" ).c_str() );
        if ( !ss.eof() )
        {
            ss >> val;
        }
        else
        {
            return false;
        }

        PG_ASSERT( !in.fail() && !ss.fail() );
        return true;
    }

    inline bool ParseLineKeyValOptional( std::istream& in, const char* key, bool& val )
    {
        PG_MAYBE_UNUSED( key );
        std::string line;
        std::string s;
        std::getline( in, line );
        auto ss = std::istringstream( line );
        ss >> s;
        PG_ASSERT( s == key, ( "Expecting key '" + std::string( key ) + "' while parsing file" ).c_str() );
        if ( !ss.eof() )
        {
            ss >> std::boolalpha >> val;
        }
        else
        {
            return false;
        }

        PG_ASSERT( !in.fail() && !ss.fail() );
        return true;
    }

    template <typename T>
    bool ParseLineKeyMap( std::istream& in, const char* key, const std::unordered_map<std::string, T>& map, T& val )
    {
        PG_MAYBE_UNUSED( key );
        std::string line;
        std::string s;
        std::getline( in, line );
        auto ss = std::istringstream( line );
        ss >> s;
        PG_ASSERT( s == key, ( "Expecting key '" + std::string( key ) + "' while parsing file" ).c_str() );
        ss >> s;
        PG_ASSERT( !in.fail() && !ss.fail() );
        auto it = map.find( s );
        if ( it == map.end() )
        {
            return false;
        }

        val = it->second;
        return true;
    }

} // namespace fileIO
