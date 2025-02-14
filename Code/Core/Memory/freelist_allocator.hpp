#pragma once
#include "memory.hpp"
#include "Structures/rb_tree.hpp"

// Always initialize and when memory is not given finalize this allocator
class FreeListAllocator
{
private:
    static constexpr UInt64 MEMORY_INITIAL_OFFSET = 8_B;
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

    Void initialize(const UInt64 bytes)
    {
        constexpr UInt64 ROOT_MEMORY_OFFSET = MEMORY_INITIAL_OFFSET + sizeof(RBNode);
        capacity = align_memory(bytes + ROOT_MEMORY_OFFSET);
        memory = VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!memory)
        {
            SPDLOG_CRITICAL("Allocation failed!");
            assert(false);
        }
        freeBlocks = { memory };
        RBNode *root = reinterpret_cast<RBNode*>(reinterpret_cast<UInt8*>(memory) + MEMORY_INITIAL_OFFSET);
        *root = {};
        root->set_size(capacity - ROOT_MEMORY_OFFSET);
        freeBlocks.insert(root);
        isIndependent = true;
        freeBlocks.printTree();
    }

    Void initialize(const UInt64 bytes, const AllocatorInfo &allocatorInfo)
    {
        parentInfo = allocatorInfo;

        if (!parentInfo.allocator)
        {
            SPDLOG_WARN("Parent allocator is nullptr!");
            return;
        }
        constexpr UInt64 ROOT_MEMORY_OFFSET = MEMORY_INITIAL_OFFSET + sizeof(RBNode);

        capacity = bytes + ROOT_MEMORY_OFFSET;
        memory = parentInfo.allocate(parentInfo.allocator, capacity);
        freeBlocks = { memory };
        RBNode *root = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + MEMORY_INITIAL_OFFSET);
        *root = {};
        root->set_size(capacity - ROOT_MEMORY_OFFSET);
        freeBlocks.insert(root);
        isIndependent = false;
    }


    Void *allocate(const UInt64 bytes)
    {
        RBNode *data = freeBlocks.find(bytes);
        freeBlocks.remove(data);
        RBNode *splitNode = freeBlocks.split_node(data, bytes);
        if (splitNode)
        {
            freeBlocks.insert(splitNode);
        }
        freeBlocks.printTree();
        return data->get_memory();
    }

    template <typename Type>
    Type *allocate(const UInt64 count)
    {
        return reinterpret_cast<Type *>(allocate(count * sizeof(Type)));
    }

    Void deallocate(Void *pointer, UInt64 bytes)
    {
        if (reinterpret_cast<UInt8 *>(memory) + capacity <= reinterpret_cast<UInt8 *>(pointer))
        {
            SPDLOG_WARN("Pointer out of scope!");
            return;
        }
        RBNode *node = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(pointer) - sizeof(RBNode));
        freeBlocks.insert(node);
        freeBlocks.printTree();
    }

    template <typename Type>
    Void deallocate(Type *pointer)
    {
        deallocate(pointer, sizeof(Type));
    }

    template <>
    Void deallocate(Void *pointer)
    {
        deallocate(pointer, 0);
    }

    Void copy(const FreeListAllocator &original)
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
        // initialize(original.capacity / blockSize, blockSize);
        memcpy(memory, original.memory, capacity);
    }

    Void move(FreeListAllocator &original)
    {
        if (this == &original)
        {
            SPDLOG_WARN("Attempted to copy StackAllocator into itself. Skipping.");
            return;
        }
        finalize();
        // memory = original.memory;
        // freeList = original.freeList;
        // capacity = original.capacity;
        // blockSize = original.blockSize;
        // isIndependent = original.isIndependent;
        // original.memory = nullptr;
        // original.freeList = nullptr;
        // original.capacity = 0;
        // original.blockSize = 0;
        // original.isIndependent = false;
    }

    Void finalize()
    {
        if (isIndependent)
        {
            VirtualFree(memory, 0, MEM_RELEASE);
            memory = nullptr;
            freeBlocks.clear();
        }
        if (parentInfo.allocator)
        {
            parentInfo.deallocate(parentInfo.allocator, memory, capacity);
            memory = nullptr;
            freeBlocks.clear();
            parentInfo.allocator = nullptr;
        }
    }

    AllocatorInfo get_allocator_info()
    {
        AllocatorInfo info;
        info.allocator = this;
        info.allocate = [](Void *allocator, UInt64 bytes) -> Void *
        {
            return static_cast<FreeListAllocator *>(allocator)->allocate(bytes);
        };

        info.deallocate = [](Void *allocator, Void *pointer, UInt64 bytes) -> Void
        {
            static_cast<FreeListAllocator *>(allocator)->deallocate(pointer, bytes);
        };
        return info;
    }
};