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
    Void          *memory;
    PoolBlock     *freeList;
    UInt64        capacity;
    UInt64        blockSize;

public:
    PoolAllocator()
        : selfInfo({})
        , parentInfo(nullptr)
        , memory(nullptr)
        , freeList(nullptr)
        , capacity(0)
        , blockSize(0)
    {}

    Void initialize(UInt64 count, UInt64 size);
    Void initialize(UInt64 count, UInt64 size, AllocatorInfo *allocatorInfo);

    Void *allocate(UInt64 bytes);
    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type*>(allocate(count * sizeof(Type)));
    }

    Void deallocate(Void *pointer);

    Void copy(const PoolAllocator &source);

    Void move(PoolAllocator &source);

    Void finalize();

    AllocatorInfo *get_allocator_info();
};