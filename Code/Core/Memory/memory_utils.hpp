#pragma once
//TODO: Think of linux support
#if defined(_WIN32)
#include <windows.h>
static constexpr UInt64 GET_PAGE_SIZE()
{
    return 4096;
}
#endif
using Byte = UInt8;

constexpr UInt64 align_memory(const UInt64 bytes)
{
    return (bytes + GET_PAGE_SIZE() - 1) & ~(GET_PAGE_SIZE() - 1);
}

constexpr UInt64 operator""_B(const UInt64 value)
{
    return UInt64(value);
}

constexpr UInt64 operator""_KiB(const UInt64 value)
{
    return value * 1024_B;
}

constexpr UInt64 operator""_MiB(const UInt64 value)
{
    return value * 1024_KiB;
}

constexpr UInt64 operator""_GiB(const UInt64 value)
{
    return value * 1024_MiB;
}

constexpr UInt64 operator""_TiB(const UInt64 value)
{
    return value * 1024_GiB;
}

struct AllocatorInfo
{
    using Allocate = Void*(*)(Void *allocator, UInt64 bytes);
    using Deallocate = Void(*)(Void *allocator, Void *pointer);
    Void *allocator;
    Allocate allocate;
    Deallocate deallocate;
};