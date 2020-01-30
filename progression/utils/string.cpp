#include "utils/string.hpp"

#include <algorithm>

std::string TrimWhiteSpace( const std::string& s )
{
    size_t start = s.find_first_not_of( " \t" );
    size_t end   = s.find_last_not_of( " \t" );
    return s.substr( start, end - start + 1 );
}

std::string SlashesToUnderscores( std::string s )
{
    std::replace( s.begin(), s.end(), '\\', '_' );
    std::replace( s.begin(), s.end(), '/', '_' );
    return s;
}

bool IsWhiteSpace( const std::string& str )
{
    for ( size_t i = 0; i < str.length(); ++i )
    {
        if ( !std::isspace( str[i] ) )
        {
            return false;
        }
    }

    return true;
}