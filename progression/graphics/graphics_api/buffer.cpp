#include "graphics/graphics_api/buffer.hpp"
#include "core/assert.hpp"
#include "core/core_defines.hpp"

namespace Progression
{
namespace Gfx
{

    int SizeOfIndexType( IndexType type )
    {
        int size[] =
        {
            2, // UNSIGNED_SHORT
            4, // UNSIGNED_INT
        };

        static_assert( ARRAY_COUNT( size ) == static_cast< int >( IndexType::NUM_INDEX_TYPE ) );

        return size[static_cast< int >( type )];
    }

    void* Buffer::Map()
    {
        void* data;
        vkMapMemory( m_device, m_memory, 0, m_length, 0, &data );
        return data;
    }

    void Buffer::UnMap()
    {
        vkUnmapMemory( m_device, m_memory );
    }

    void Buffer::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyBuffer( m_device, m_handle, nullptr );
        vkFreeMemory( m_device, m_memory, nullptr );
        m_handle = VK_NULL_HANDLE;
    }

    size_t Buffer::GetLength() const
    {
        return m_length;
    }

    BufferType Buffer::GetType() const
    {
        return m_type;
    }

    MemoryType Buffer::GetMemoryType() const
    {
        return m_memoryType;
    }

    VkBuffer Buffer::GetHandle() const
    {
        return m_handle;
    }

    Buffer::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

    void Buffer::Bind( size_t offset ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE && m_memory != VK_NULL_HANDLE );
        vkBindBufferMemory( m_device, m_handle, m_memory, offset );
    }

} // namespace Gfx
} // namespace Progression