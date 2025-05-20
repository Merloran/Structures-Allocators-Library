#pragma once
#include "Memory/memory_utils.hpp"

// Always initialize and when memory is not given finalize this allocator
class StackAllocator
{
private:
    AllocatorInfo selfInfo;
    AllocatorInfo *parentInfo;
    Byte          *memory;
    USize          capacity;
    USize          offset;

public:
    StackAllocator() noexcept
        : selfInfo({})
        , parentInfo(nullptr)
        , memory(nullptr)
        , capacity(0)
        , offset(0)
    {}

    Void initialize(USize bytes) noexcept;
    Void initialize(USize bytes, AllocatorInfo *allocatorInfo) noexcept;

    [[nodiscard]]
    Byte *allocate(USize bytes, USize alignment) noexcept;
    template <typename Type>
    [[nodiscard]]
    Type *allocate() noexcept
    {
        return new (allocate(sizeof(Type), alignof(Type))) Type();
    }
    template <typename Type>
    [[nodiscard]]
    Type *allocate(const USize count) noexcept
    {
        Type *mem = reinterpret_cast<Type *>(allocate(count * sizeof(Type), alignof(Type)));

        for (USize i = 0; i < count; ++i)
        {
            new (&mem[i]) Type();
        }

        return mem;
    }


    Void deallocate(USize marker = 0) noexcept;
    Void deallocate(Byte *pointer) noexcept;
    template <typename Type>
    Void deallocate(Type *pointer) noexcept
    {
        deallocate(byte_cast(pointer));
    }

    Void copy(const StackAllocator &source) noexcept;

    Void move(StackAllocator &source) noexcept;

    Void finalize() noexcept;

    AllocatorInfo *get_allocator_info() noexcept;
};