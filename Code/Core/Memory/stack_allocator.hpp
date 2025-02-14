#pragma once
#include "Memory/memory_utils.hpp"

// Always initialize and when memory is not given finalize this allocator
class StackAllocator
{
private:
    AllocatorInfo  parentInfo;
    Void          *memory;
    UInt64         capacity;
    UInt64         offset;
    Bool           isIndependent;

public:
    StackAllocator()
        : parentInfo({})
        , memory(nullptr)
        , capacity(0)
        , offset(0)
        , isIndependent(false)
    {}

    Void initialize(UInt64 bytes);

    Void initialize(UInt64 bytes, const AllocatorInfo &allocatorInfo);

    Void *allocate(UInt64 bytes);
    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type*>(allocate(count * sizeof(Type)));
    }

    Void deallocate(UInt64 marker = 0);
    Void deallocate(Void *pointer);

    Void copy(const StackAllocator &source);

    Void move(StackAllocator &source);

    Void finalize();

    AllocatorInfo get_allocator_info();
};