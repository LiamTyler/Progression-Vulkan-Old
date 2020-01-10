#pragma once

namespace Progression
{
namespace MemoryManager
{

    void* Allocate( size_t bytes );
    void Free( void* ptr ) noexcept;

    template < typename T, typename... Args >
    T* New( Args&&... args )
    {
        void* mem = Allocate( sizeof( T ) );
        T* ptr = new( mem ) T( std::forward< Args >( args )... );
        return ptr;
    }

    template < typename T >
    void Delete( T* ptr )
    {
        ptr->~T();
        Free( ptr );
    }
 
    template < class T >
    struct Allocator
    {
        typedef T value_type;

        Allocator() = default;
        template < class U >
        constexpr Allocator(const Allocator< U >& ) noexcept {}

        T* allocate( std::size_t n )
        {
            std::cout << "allocator allocating: " << n << " elements" << std::endl;
            return static_cast< T* >( MemoryManager::Allocate( n * sizeof( T ) ) );
        }

        void deallocate( T* p, std::size_t ) noexcept
        {
            std::cout << "allocator freeing!" << std::endl;
            MemoryManager::Free( p );
        }
    };
 
    template < class T, class U >
    bool operator==( const Allocator< T >&, const Allocator< U >& )
    {
        return true;
    }
    template < class T, class U >
    bool operator!=( const Allocator< T >&, const Allocator< U >& )
    {
        return false;
    }
    
} // namespace MemoryManager
} // namespace Progression