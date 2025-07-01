#pragma once
#include "Memory/memory_utils.hpp"
#include "Structures/rb_tree.hpp"

// Always initialize and when memory is not given finalize this allocator
class FreeListAllocator
{
private:
    RBTree         freeBlocks;
    AllocatorInfo  selfInfo;
    AllocatorInfo *parentInfo;
    Byte          *memory;
    USize          capacity;

public:
    FreeListAllocator() noexcept
        : selfInfo({})
        , parentInfo(nullptr)
        , memory(nullptr)
        , capacity(0)
    {}

    Void initialize(USize bytes) noexcept;
    Void initialize(USize bytes, AllocatorInfo *allocatorInfo) noexcept;

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

    //Copy size and optionally takes same parent allocator
    Void copy(const FreeListAllocator &source) noexcept;

    Void move(FreeListAllocator &source) noexcept;

    Void print_list() noexcept;

    Void finalize() noexcept;

    AllocatorInfo *get_allocator_info() noexcept;
};