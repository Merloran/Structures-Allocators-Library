#pragma once
#include "Memory/memory_utils.hpp"
#include "Structures/rb_tree.hpp"

// Always initialize and when memory is not given finalize this allocator
class FreeListAllocator
{
private:
    static constexpr UInt64 INITIAL_MEMORY_OFFSET = 8_B;
    RBTree        freeBlocks;
    AllocatorInfo selfInfo;
    AllocatorInfo *parentInfo;
    Void          *memory;
    UInt64        capacity;

public:
    FreeListAllocator()
        : selfInfo({})
        , parentInfo(nullptr)
        , memory(nullptr)
        , capacity(0)
    {}

    Void initialize(UInt64 bytes);
    Void initialize(UInt64 bytes, AllocatorInfo *allocatorInfo);

    Void *allocate(UInt64 bytes);
    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type *>(allocate(count * sizeof(Type)));
    }

    Void deallocate(Void *pointer);

    //Copy size and optionally takes same parent allocator
    Void copy(const FreeListAllocator &source);

    Void move(FreeListAllocator &source);

    Void print_list();

    Void finalize();

    AllocatorInfo *get_allocator_info();
};