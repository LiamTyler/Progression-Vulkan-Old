#pragma once

#include "glm/glm.hpp"
#include <fstream>
#include <vector>
#include <iostream>

namespace serialize
{

template < typename T >
inline void Write( std::ofstream& out, const T& val )
{
    out.write( (char*)&val, sizeof( T ) );
}

template <>
inline void Write( std::ofstream& out, const std::string& s )
{
    uint32_t len = s.length();
    out.write( (char*)&len, sizeof( uint32_t ) );
    out.write( s.c_str(), len );
}

template <>
inline void Write( std::ofstream& out, const glm::vec3& v )
{
    out.write( (char*)&v.x, sizeof( glm::vec3 ) );
}

template < typename T >
inline void Write( std::ofstream& out, const std::vector< T >& vec )
{
    size_t len = vec.size();
    serialize::Write( out, len );
    if ( len )
        out.write( (char*)&vec[0], len * sizeof( T ) );
}

inline void Write( std::ofstream& out, char* buff, size_t len )
{
    out.write( buff, len );
}

template < typename T >
inline void Read( std::ifstream& in, T& val )
{
    in.read( (char*)&val, sizeof( T ) );
}

template <>
inline void Read( std::ifstream& in, std::string& s )
{
    uint32_t len;
    in.read( (char*)&len, sizeof( uint32_t ) );
    s.resize( len );
    in.read( (char*)&s[0], len );
}

template <>
inline void Read( std::ifstream& in, glm::vec3& v )
{
    in.read( (char*)&v.x, sizeof( glm::vec3 ) );
}

template < typename T >
inline void Read( std::ifstream& in, std::vector< T >& vec )
{
    size_t len;
    serialize::Read( in, len );
    vec.resize( len );
    if ( len )
        in.read( (char*)&vec[0], len * sizeof( T ) );
}

inline void Read( std::ifstream& in, char* buff, size_t len )
{
    in.read( buff, len );
}



template < typename T >
inline void Read( char*& buff, T& val )
{
    //PG_ASSERT( buff );
    memcpy( (void*) &val, buff, sizeof( T ) );
    buff += sizeof( T );
}

template <>
inline void Read( char*& buff, std::string& s )
{
    //PG_ASSERT( buff );
    uint32_t len = ( ( uint32_t* ) buff )[0];
    buff += sizeof( uint32_t );
    s.resize( len );
    memcpy( &s[0], buff, len );
    buff += len;
}

template <>
inline void Read( char*& buff, glm::vec3& v )
{
    //PG_ASSERT( buff );
    memcpy( (void*) &v.x, buff, sizeof( glm::vec3 ) );
    buff += sizeof( glm::vec3 );
}

template < typename T >
inline void Read( char*& buff, std::vector< T >& vec )
{
    //PG_ASSERT( buff );
    size_t len = ( ( size_t* ) buff )[0];
    buff += sizeof( size_t );
    std::cout << len << std::endl;
    vec.resize( len );
    memcpy( vec.data(), buff, len * sizeof( T ) );
    buff += len * sizeof( T );
}

inline void Read( char*& buff, char* dstBuff, size_t len )
{
    //PG_ASSERT( buff );
    memcpy( dstBuff, buff, len );
    buff += len;
}

} // namespace serialize