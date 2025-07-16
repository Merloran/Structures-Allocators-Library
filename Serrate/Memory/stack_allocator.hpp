#pragma once
#include "memory_utils.hpp"

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


    Void deallocate(USize marker = 0) noexcept;
    Void deallocate(Byte *pointer) noexcept;
    template <Manual Type>
    Void deallocate(Type *pointer) noexcept
    {
        deallocate(byte_cast(pointer));
    }

    Void copy(const StackAllocator &source) noexcept;

    Void move(StackAllocator &source) noexcept;

    [[nodiscard]]
    USize get_capacity() const noexcept;

    Void finalize() noexcept;

    AllocatorInfo *get_allocator_info() noexcept;
};