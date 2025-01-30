#pragma once
#include "memory.hpp"

struct FreeBlock
{
    FreeBlock *next;
};

// Always initialize and when memory is not given finalize this allocator
class PoolAllocator
{
private:
    AllocatorInfo  parentInfo;
    Void          *memory;
    FreeBlock     *freeList;
    UInt64         capacity;
    UInt64         blockSize;
    Bool           isIndependent;

public:
    PoolAllocator()
        : parentInfo({})
        , memory(nullptr)
        , freeList(nullptr)
        , capacity(0)
        , blockSize(0)
        , isIndependent(false)
    {}

    Void initialize(const UInt64 count, const UInt64 size)
    {
        blockSize = size;
        capacity = align_memory(count * size);
        memory = VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!memory)
        {
            SPDLOG_CRITICAL("Allocation failed!");
            assert(false);
        }

        freeList = static_cast<FreeBlock *>(memory);
        FreeBlock *current = freeList;
        for (UInt64 i = 0; i < count; ++i)
        {
            current->next = reinterpret_cast<FreeBlock *>(reinterpret_cast<UInt8 *>(memory) + i * blockSize);
            current = current->next;
        }
        current->next = nullptr;

        isIndependent = true;
    }

    Void initialize(const UInt64 count, const UInt64 size, const AllocatorInfo &allocatorInfo)
    {
        blockSize = size;
        parentInfo = allocatorInfo;

        if (!parentInfo.allocator)
        {
            SPDLOG_WARN("Parent allocator is nullptr!");
            return;
        }
        
        capacity = count * size;
        memory   = parentInfo.allocate(parentInfo.allocator, capacity);
        isIndependent = false;
    }


    Void *allocate(const UInt64 bytes)
    {
        if (bytes > blockSize)
        {
            SPDLOG_CRITICAL("Requested too much memory! || Requested: {} || Max: {}", bytes, blockSize);
            assert(false);
            return nullptr;
        }

        if (!freeList)
        {
            SPDLOG_CRITICAL("Out of memory! || CAPACITY: {}", capacity);
            assert(false);
            return nullptr;
        }
        Void *address = freeList;
        freeList = freeList->next;
        return address;
    }

    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type*>(allocate(count * sizeof(Type)));
    }

    template <typename Type>
    Void deallocate(Type *pointer)
    {
        if (reinterpret_cast<UInt8*>(memory) + capacity <= reinterpret_cast<UInt8*>(pointer))
        {
            SPDLOG_WARN("Pointer out of scope!");
            return;
        }
        UInt64 pointerAlignment = (reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory)) % blockSize;
        if (pointerAlignment != 0)
        {
            pointer = reinterpret_cast<Type*>(reinterpret_cast<UInt8 *>(pointer) - pointerAlignment);
        }
        FreeBlock *freeBlock = static_cast<FreeBlock *>(pointer);
        freeBlock->next = freeList;
        freeList = freeBlock;
    }

    Void deallocate(Void *pointer, UInt64 bytes)
    {
        if (reinterpret_cast<UInt8 *>(memory) + capacity <= reinterpret_cast<UInt8 *>(pointer))
        {
            SPDLOG_WARN("Pointer out of scope!");
            return;
        }
        if (bytes > blockSize)
        {
            SPDLOG_WARN("Requested size is too big: {}", bytes);
            return;
        }

        UInt64 pointerAlignment = (reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory)) % blockSize;
        if (pointerAlignment != 0)
        {
            pointer = reinterpret_cast<Void *>(reinterpret_cast<UInt8 *>(pointer) - pointerAlignment);
        }

        FreeBlock *freeBlock = static_cast<FreeBlock *>(pointer);
        freeBlock->next = freeList;
        freeList = freeBlock;
    }

    Void copy(const PoolAllocator &original)
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
        initialize(original.capacity / blockSize, blockSize);
        memcpy(memory, original.memory, capacity);
    }

    Void move(PoolAllocator &original)
    {
        if (this == &original)
        {
            SPDLOG_WARN("Attempted to copy StackAllocator into itself. Skipping.");
            return;
        }
        finalize();
        memory = original.memory;
        freeList = original.freeList;
        capacity = original.capacity;
        blockSize = original.blockSize;
        isIndependent = original.isIndependent;
        original.memory = nullptr;
        original.freeList = nullptr;
        original.capacity = 0;
        original.blockSize = 0;
        original.isIndependent = false;
    }

    Void finalize()
    {
        if (isIndependent)
        {
            VirtualFree(memory, 0, MEM_RELEASE);
            memory = nullptr;
            freeList = nullptr;
        }
        if (parentInfo.allocator)
        {
            parentInfo.deallocate(parentInfo.allocator, memory, capacity);
            memory = nullptr;
            freeList = nullptr;
            parentInfo.allocator = nullptr;
        }
    }

    AllocatorInfo get_allocator_info()
    {
        AllocatorInfo info;
        info.allocator = this;
        info.allocate = [](Void *allocator, UInt64 bytes) -> Void *
        {
            return static_cast<PoolAllocator *>(allocator)->allocate(bytes);
        };

        info.deallocate = [](Void *allocator, Void *pointer, UInt64 bytes) -> Void
        {
            static_cast<PoolAllocator *>(allocator)->deallocate(pointer, bytes);
        };
        return info;
    }
};