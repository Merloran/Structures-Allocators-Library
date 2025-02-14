#pragma once
#include "Memory/memory_utils.hpp"
#include "Structures/rb_tree.hpp"

// Always initialize and when memory is not given finalize this allocator
class FreeListAllocator
{
private:
    static constexpr UInt64 INITIAL_MEMORY_OFFSET = 8_B;
    AllocatorInfo parentInfo;
    RBTree        freeBlocks;
    Void          *memory;
    UInt64        capacity;
    Bool          isIndependent;

public:
    FreeListAllocator()
        : parentInfo({})
        , memory(nullptr)
        , capacity(0)
        , isIndependent(false)
    {}

    Void initialize(UInt64 bytes);
    Void initialize(UInt64 bytes, const AllocatorInfo &allocatorInfo);

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

    Void finalize();

    AllocatorInfo get_allocator_info();
};