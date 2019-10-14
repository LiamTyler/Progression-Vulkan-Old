#pragma once

#include <string>
#include <iostream>
#include <ctime>

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

    friend std::ostream& operator<<( std::ostream& out, const Timestamp& ts );

    time_t time = 0;
};
