#include "utils/timestamp.hpp"
#include <sys/stat.h>

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