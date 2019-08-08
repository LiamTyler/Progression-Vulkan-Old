#pragma once

#include <string>

class Timestamp
{
public:
    Timestamp() = default;
    Timestamp( const std::string& file );

    bool operator<( const Timestamp& t ) const;
    bool operator>( const Timestamp& t ) const;
    bool operator<=( const Timestamp& t ) const;
    bool operator>=( const Timestamp& t ) const;
    bool operator==( const Timestamp& t ) const;
    bool operator!=( const Timestamp& t ) const;
    operator time_t() const;

    time_t time = 0;
};
