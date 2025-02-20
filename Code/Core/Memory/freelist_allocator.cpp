#include "freelist_allocator.hpp"

#include "Structures/rb_node.hpp"


Void FreeListAllocator::initialize(const UInt64 bytes)
{
    constexpr UInt64 ROOT_MEMORY_OFFSET = INITIAL_MEMORY_OFFSET + sizeof(RBNode);
    capacity = align_memory(bytes + ROOT_MEMORY_OFFSET);
    memory = VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory)
    {
        SPDLOG_CRITICAL("Allocation failed!");
        assert(false);
    }

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, UInt64 bytes) -> Void *
    {
        return static_cast<FreeListAllocator *>(allocator)->allocate(bytes);
    };

    selfInfo.deallocate = [](Void *allocator, Void *pointer) -> Void
    {
        static_cast<FreeListAllocator *>(allocator)->deallocate(pointer);
    };

    freeBlocks = { memory };
    RBNode *root = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + INITIAL_MEMORY_OFFSET);
    *root = {};
    root->set_size(capacity - ROOT_MEMORY_OFFSET);
    freeBlocks.insert(root);
}

Void FreeListAllocator::initialize(const UInt64 bytes, AllocatorInfo *allocatorInfo)
{
    parentInfo = allocatorInfo;

    if (!parentInfo)
    {
        SPDLOG_WARN("Parent allocator is nullptr!");
        return;
    }

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, UInt64 bytes) -> Void *
    {
        return static_cast<FreeListAllocator *>(allocator)->allocate(bytes);
    };

    selfInfo.deallocate = [](Void *allocator, Void *pointer) -> Void
    {
        static_cast<FreeListAllocator *>(allocator)->deallocate(pointer);
    };

    constexpr UInt64 ROOT_MEMORY_OFFSET = INITIAL_MEMORY_OFFSET + sizeof(RBNode);

    capacity = bytes + ROOT_MEMORY_OFFSET;
    memory = parentInfo->allocate(parentInfo->allocator, capacity);
    freeBlocks = { memory };
    RBNode *root = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + INITIAL_MEMORY_OFFSET);
    *root = {};
    root->set_size(capacity - ROOT_MEMORY_OFFSET);
    freeBlocks.insert(root);
}

Void* FreeListAllocator::allocate(const UInt64 bytes)
{
    RBNode *data = freeBlocks.find(bytes);
    freeBlocks.remove(data);
    RBNode *splitNode = freeBlocks.split_node(data, bytes);
    if (splitNode)
    {
        freeBlocks.insert(splitNode, false);
    }
    return data->get_memory();
}

Void FreeListAllocator::deallocate(Void* pointer)
{
    if (reinterpret_cast<UInt8 *>(memory) + capacity <= reinterpret_cast<UInt8 *>(pointer))
    {
        SPDLOG_WARN("Pointer out of scope!");
        return;
    }
    RBNode *node = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(pointer) - sizeof(RBNode));
    freeBlocks.insert(node);
}

Void FreeListAllocator::copy(const FreeListAllocator& source)
{
    if (this == &source)
    {
        SPDLOG_WARN("Attempted to copy allocator into itself. Skipping.");
        return;
    }

    if (source.memory == nullptr)
    {
        SPDLOG_WARN("Copying from an empty allocator. Destination will also be empty.");
        return;
    }

    constexpr UInt64 ROOT_MEMORY_OFFSET = INITIAL_MEMORY_OFFSET + sizeof(RBNode);
    finalize();
    if (!source.parentInfo)
    {
        initialize(source.capacity - ROOT_MEMORY_OFFSET);
    } else {
        initialize(source.capacity - ROOT_MEMORY_OFFSET, source.parentInfo);
    }

}

Void FreeListAllocator::move(FreeListAllocator& source)
{
    if (this == &source)
    {
        SPDLOG_WARN("Attempted to move allocator into itself. Skipping.");
        return;
    }
    finalize();
    parentInfo    = source.parentInfo;
    freeBlocks    = source.freeBlocks;
    memory        = source.memory;
    capacity      = source.capacity;

    source = {};
}

Void FreeListAllocator::print_list()
{
    RBNode *node = reinterpret_cast<RBNode*>(reinterpret_cast<UInt8*>(memory) + INITIAL_MEMORY_OFFSET);
    while (node)
    {
        printf("%llu(%s)->", node->get_size(), node->is_free() ? "free" : "reserved");
        node = node->get_next();
    }
    printf("\n");
}

Void FreeListAllocator::finalize()
{
    if (!memory)
    {
        *this = {};
        return;
    }

    if (!parentInfo)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    } else {
        parentInfo->deallocate(parentInfo->allocator, memory);
    }
    *this = {};
}

AllocatorInfo *FreeListAllocator::get_allocator_info()
{
    return &selfInfo;
}