#pragma once


#define DEFINE_ENUM_BITWISE_OPERATORS( E ) \
inline E operator | ( E lhs, E rhs ) \
{ \
    using T = std::underlying_type_t< E >; \
    return static_cast< E >( static_cast< T >( lhs ) | static_cast< T >( rhs ) ); \
} \
\
inline E& operator |= ( E& lhs, E rhs ) \
{ \
    lhs = lhs | rhs; \
    return lhs; \
} \
\
inline E operator & ( E lhs, E rhs ) \
{ \
    using T = std::underlying_type_t< E >; \
    return static_cast< E >( static_cast< T >( lhs ) & static_cast< T >( rhs ) ); \
} \
\
inline E& operator &= ( E& lhs, E rhs ) \
{ \
    lhs = lhs & rhs; \
    return lhs; \
} \
\
inline E operator ^ ( E lhs, E rhs ) \
{ \
    using T = std::underlying_type_t< E >; \
    return static_cast< E >( static_cast< T >( lhs ) ^ static_cast< T >( rhs ) ); \
} \
\
inline E& operator ^= ( E& lhs, E rhs ) \
{ \
    lhs = lhs ^ rhs; \
    return lhs; \
} \
\
inline E operator ~ ( E rhs ) \
{ \
    using T = std::underlying_type_t< E >; \
    return static_cast< E >( ~static_cast< T >( rhs ) ); \
}