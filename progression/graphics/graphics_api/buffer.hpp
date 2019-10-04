#pragma once

#include "core/enum_bit_operations.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    enum class BufferDataType
    {
        INVALID = 0,

        UCHAR  = 1,
        UCHAR2 = 2,
        UCHAR3 = 3,
        UCHAR4 = 4,

        CHAR  = 5,
        CHAR2 = 6,
        CHAR3 = 7,
        CHAR4 = 8,

        UCHAR_NORM  = 9,
        UCHAR2_NORM = 10,
        UCHAR3_NORM = 11,
        UCHAR4_NORM = 12,

        CHAR_NORM  = 13,
        CHAR2_NORM = 14,
        CHAR3_NORM = 15,
        CHAR4_NORM = 16,

        USHORT  = 17,
        USHORT2 = 18,
        USHORT3 = 19,
        USHORT4 = 20,

        SHORT  = 21,
        SHORT2 = 22,
        SHORT3 = 23,
        SHORT4 = 24,

        USHORT_NORM  = 25,
        USHORT2_NORM = 26,
        USHORT3_NORM = 27,
        USHORT4_NORM = 28,

        SHORT_NORM  = 29,
        SHORT2_NORM = 30,
        SHORT3_NORM = 31,
        SHORT4_NORM = 32,

        HALF  = 33,
        HALF2 = 34,
        HALF3 = 35,
        HALF4 = 36,

        FLOAT  = 37,
        FLOAT2 = 38,
        FLOAT3 = 39,
        FLOAT4 = 40,

        UINT  = 41,
        UINT2 = 42,
        UINT3 = 43,
        UINT4 = 44,

        INT  = 45,
        INT2 = 46,
        INT3 = 47,
        INT4 = 48,

        NUM_BUFFER_DATA_TYPE
    };

    enum class IndexType
    {
        UNSIGNED_SHORT = 0,
        UNSIGNED_INT,

        NUM_INDEX_TYPE
    };

    int SizeOfIndexType( IndexType type );

    enum class BufferType
    {
        TRANSFER_SRC  = 1 << 0,
        TRANSFER_DST  = 1 << 1,
        UNIFORM_TEXEL = 1 << 2,
        STORAGE_TEXEL = 1 << 3,
        UNIFORM       = 1 << 4,
        STORAGE       = 1 << 5,
        INDEX         = 1 << 6,
        VERTEX        = 1 << 7,
        INDIRECT      = 1 << 8,

        NUM_BUFFER_TYPE = 9
    };
    DEFINE_ENUM_BITWISE_OPERATORS( BufferType );

    enum class MemoryType
    {
        DEVICE_LOCAL  = 1 << 0,
        HOST_VISIBLE  = 1 << 1,
        HOST_COHERENT = 1 << 2,
        HOST_CACHED   = 1 << 3,
    };
    DEFINE_ENUM_BITWISE_OPERATORS( MemoryType );

    class Buffer
    {
    friend class Device;
    public:
        void Free();
        void* Map();
        void UnMap();
        void Bind( size_t offset = 0 ) const;
        size_t GetLength() const;
        MemoryType GetMemoryType() const;
        BufferType GetType() const;
        VkBuffer GetNativeHandle() const;
        operator bool() const;

    protected:
        BufferType m_type;
        MemoryType m_memoryType;
        size_t m_length         = 0; // in bytes
        VkBuffer m_handle       = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDevice m_device       = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression