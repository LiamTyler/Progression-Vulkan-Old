#include "utils/timestamp.hpp"
#include <sys/stat.h>
#include <iomanip>
#include "core/platform_defines.hpp"

Timestamp::Timestamp( const std::string& file )
{
    struct stat s;
    if ( stat( file.c_str(), &s ) != 0 )
    {
        time = 0;
    }
    else
    {
        time = s.st_mtime;
    }
}

bool Timestamp::operator<( const Timestamp& t ) const
{
    return time < t.time;
}

bool Timestamp::operator>( const Timestamp& t ) const
{
    return time > t.time;
}

bool Timestamp::operator<=( const Timestamp& t ) const
{
    return !( time > t.time );
}

bool Timestamp::operator>=( const Timestamp& t ) const
{
    return !( time < t.time );
}

bool Timestamp::operator==( const Timestamp& t ) const
{
    return time == t.time;
}

bool Timestamp::operator!=( const Timestamp& t ) const
{
    return !( time == t.time );
}

Timestamp::operator time_t() const
{
    return time;
}

std::ostream& operator<<( std::ostream& out, const Timestamp& ts )
{
#if USING( WINDOWS_PROGRAM )
    std::tm tm;
    localtime_s( &tm, &ts.time );
    return out << std::put_time( &tm, "%c" );
#else // #if USING( WINDOWS_PROGRAM )
    auto tm = std::localtime( &ts.time );
    return out << std::put_time( tm, "%c" );
#endif // #else // #if USING( WINDOWS_PROGRAM )
}