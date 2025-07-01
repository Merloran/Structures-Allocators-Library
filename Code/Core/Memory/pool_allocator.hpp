#pragma once
#include "Memory/memory_utils.hpp"

struct PoolBlock
{
    PoolBlock *next;
};

// Always initialize and when memory is not given finalize this allocator
class PoolAllocator
{
private:
    AllocatorInfo selfInfo;
    AllocatorInfo *parentInfo;
    Byte          *memory;
    PoolBlock     *freeList;
    USize        capacity;
    USize        blockSize;

public:
    PoolAllocator()
        : selfInfo({})
        , parentInfo(nullptr)
        , memory(nullptr)
        , freeList(nullptr)
        , capacity(0)
        , blockSize(0)
    {}

    Void initialize(USize count, USize size) noexcept;
    Void initialize(USize count, USize size, AllocatorInfo *allocatorInfo) noexcept;

    Byte *allocate(USize bytes, USize alignment) noexcept;
    template <Manual Type>
    [[nodiscard]]
    Type *allocate() noexcept
    {
        return Memory::start_object<Type>(allocate(sizeof(Type), alignof(Type)));
    }
    template <Manual Type>
    [[nodiscard]]
    Type *allocate(const USize count) noexcept
    {
        return Memory::start_object<Type>(allocate(count * sizeof(Type), alignof(Type)), count);
    }

    Void deallocate(Byte *pointer) noexcept;
    template <Manual Type>
    Void deallocate(Type *pointer) noexcept
    {
        deallocate(byte_cast(pointer));
    }

    Void copy(const PoolAllocator &source) noexcept;

    Void move(PoolAllocator &source) noexcept;

    Void finalize() noexcept;

    AllocatorInfo *get_allocator_info() noexcept;
};