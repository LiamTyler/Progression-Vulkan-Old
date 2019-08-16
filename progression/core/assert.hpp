#pragma once

#include "platform_defines.hpp"

#if !USING( SHIP_BUILD )

#define PG_ASSERT( x )                                                                   \
if ( !( x ) )                                                                            \
{                                                                                        \
    printf( "Failed assertion: (%s) at line %d in file %s.\n", #x, __LINE__, __FILE__ ); \
    abort();                                                                             \
}

#define PG_ASSERT_WITH_MSG( x, msg )                                                                                \
if ( !( x ) )                                                                                                       \
{                                                                                                                   \
    printf( "Failed assertion: (%s) at line %d in file %s.\nAssert nessage: '%s'\n", #x, __LINE__, __FILE__, msg ); \
    abort();                                                                                                        \
}

#else // #if !USING( SHIP_BUILD )

#define PG_ASSERT( x ) do {} while ( 0 )
#define PG_ASSERT_WITH_MSG( x, msg ) do {} while ( 0 )

#endif // #else // #if !USING( SHIP_BUILD )