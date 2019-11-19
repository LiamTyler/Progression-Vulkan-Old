#include "progression.hpp"
#include "basis_universal/basisu_comp.h"

using namespace Progression;

int main( int argc, char* argv[] )
{
    if ( !PG::EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 0;
    }

    PG::EngineQuit();

    return 0;
}
