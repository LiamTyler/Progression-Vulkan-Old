#include "core/memory_manager.hpp"

namespace Progression
{
namespace MemoryManager
{
    void* Allocate( size_t bytes )
    {
        return malloc( bytes );
    }
    
    void Free( void* ptr ) noexcept
    {
        free( ptr );
    }
    
} // namespace MemoryManager
} // namespace Progression

/*
#include <iostream>
#include <vector>

using namespace Progression;

struct Point
{
    Point() = default;
    Point( float _x, float _y, float _z ) : x(_x), y(_y), z(_z) {}
    ~Point()
    {
        std::cout << "Point dtor" << std::endl;
    }

    float x = 0;
    float y = 0;
    float z = 0;
};

void Example()
{
    Point* p = MemoryManager::New< Point >( 1, 2, 3 );
    std::cout << p->x << " " << p->y << " " << p->z << std::endl;
    MemoryManager::Delete( p );
    {
        std::vector< Point, MemoryManager::Allocator< Point > > vec;
        for ( int i = 0; i < 10; ++i )
        {
            std::cout << "Vector capacity: " << vec.capacity() << std::endl;
            vec.push_back( {} );
            std::cout << "Total elements currently: " << i << std::endl;
        }
        std::cout << "clearing" << std::endl;
        vec.clear();
        std::cout << "shrinking" << std::endl;
        vec.shrink_to_fit();

        std::cout << "ending" << std::endl;
    }
    std::cout << "done" << std::endl;
}
*/