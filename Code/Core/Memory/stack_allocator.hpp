#pragma once
#include <windows.h>
#include <cstddef>

#include "memory.hpp"

// Always initialize and when memory is not given finalize this allocator
class StackAllocator
{
private:
    AllocatorInfo parentInfo;
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

    Void initialize(const UInt64 bytes)
    {
        offset = 0;
        capacity = align_memory(bytes);
        memory = VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!memory)
        {
            SPDLOG_CRITICAL("Allocation failed!");
            assert(false);
        }
        isIndependent = true;
    }

    Void initialize(const UInt64 bytes, const AllocatorInfo &allocatorInfo)
    {
        offset = 0;
        parentInfo = allocatorInfo;

        if (!parentInfo.allocator)
        {
            SPDLOG_WARN("Parent allocator is nullptr!");
            return;
        }
        
        capacity = bytes;
        memory   = parentInfo.allocate(parentInfo.allocator, capacity);
        isIndependent = false;
    }


    Void *allocate(const UInt64 bytes)
    {
        Void *address = reinterpret_cast<UInt8 *>(memory) + offset;
        offset += bytes;
        if (offset > capacity)
        {
            SPDLOG_CRITICAL("Out of memory! || CAPACITY: {} || OFFSET: {}", capacity, offset);
            assert(false);
        }
        return address;
    }

    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type*>(allocate(count * sizeof(Type)));
    }

    Void deallocate(const UInt64 marker = 0)
    {
        if (marker <= offset)
        {
            offset = marker;
        }
    }

    template <typename Type>
    Void deallocate(Type *pointer)
    {
        UInt64 marker = reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory);
        if (marker <= offset)
        {
            offset = marker;
        }
    }

    Void deallocate(Void *pointer, UInt64 bytes)
    {
        UInt64 marker = reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory);
        if (marker <= offset)
        {
            offset = marker;
        }
    }

    Void copy(const StackAllocator &original)
    {
        if (this == &original)
        {
            SPDLOG_WARN("Attempted to copy StackAllocator into itself. Skipping.");
            return;
        }

        if (original.memory == nullptr)
        {
            SPDLOG_WARN("Copying from an empty StackAllocator. Destination will also be empty.");
            return;
        }
        finalize();
        initialize(original.capacity);
        offset = original.offset;
        memcpy(memory, original.memory, offset);
    }

    Void move(StackAllocator &original)
    {
        if (this == &original)
        {
            SPDLOG_WARN("Attempted to copy StackAllocator into itself. Skipping.");
            return;
        }
        finalize();
        memory = original.memory;
        capacity = original.capacity;
        offset = original.offset;
        isIndependent = original.isIndependent;
        original.memory = nullptr;
        original.capacity = 0;
        original.offset = 0;
        original.isIndependent = false;
    }

    Void finalize()
    {
        if (isIndependent)
        {
            VirtualFree(memory, 0, MEM_RELEASE);
            memory = nullptr;
        }
        if (parentInfo.allocator)
        {
            parentInfo.deallocate(parentInfo.allocator, memory, capacity);
            memory = nullptr;
            parentInfo.allocator = nullptr;
        }
    }

    AllocatorInfo get_allocator_info()
    {
        AllocatorInfo info;
        info.allocator = this;
        info.allocate = [](Void *allocator, UInt64 bytes) -> Void *
        {
            return static_cast<StackAllocator *>(allocator)->allocate(bytes);
        };

        info.deallocate = [](Void *allocator, Void *pointer, UInt64 bytes) -> Void
        {
            static_cast<StackAllocator *>(allocator)->deallocate(pointer, bytes);
        };
        return info;
    }
};