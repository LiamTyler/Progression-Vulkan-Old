#include "utils/random.hpp"

#include <stdlib.h>
#include <time.h>

namespace Progression
{
namespace Random
{

    void SetSeed( unsigned int seed ) { srand( seed ); }

    int RandInt( int l, int h ) { return l + std::rand() % ( h - l + 1 ); }

    float RandFloat( float l, float h )
    {
        float r = std::rand() / static_cast< float >( RAND_MAX );
        return r * ( h - l ) + l;
    }

    float Rand() { return std::rand() / static_cast< float >( RAND_MAX ); }

} // namespace Random

} // namespace Progression
