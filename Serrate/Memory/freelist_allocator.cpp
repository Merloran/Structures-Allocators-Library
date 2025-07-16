#include "freelist_allocator.hpp"
#include "Serrate/Structures/rb_node.hpp"

#include <cstdio>


Void FreeListAllocator::initialize(const USize bytes) noexcept
{
    capacity = align_system_memory(bytes + sizeof(RBNode));
    memory = byte_cast(VirtualAlloc(nullptr, 
                                           capacity, 
                                           MEM_RESERVE | MEM_COMMIT, 
                                           PAGE_READWRITE));

    assert(memory != nullptr && "Allocation failed!");

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, const USize bytes, const USize alignment) -> Byte *
    {
        return static_cast<FreeListAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<FreeListAllocator *>(allocator)->deallocate(pointer);
    };

    freeBlocks = { memory };
    RBNode *root = Memory::start_object<RBNode>(memory);
    root->set_size(capacity - sizeof(RBNode));
    freeBlocks.insert(root);
}

Void FreeListAllocator::initialize(const USize bytes, AllocatorInfo *allocatorInfo) noexcept
{
    assert(allocatorInfo != nullptr && "Parent allocator is nullptr!");
    parentInfo = allocatorInfo;

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, const USize bytes, const USize alignment) -> Byte *
    {
        return static_cast<FreeListAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<FreeListAllocator *>(allocator)->deallocate(pointer);
    };

    capacity = bytes + sizeof(RBNode);
    memory = parentInfo->allocate(parentInfo->allocator, capacity, alignof(RBNode));
    freeBlocks = { memory };
    RBNode *root = Memory::start_object<RBNode>(memory);
    root->set_size(capacity - sizeof(RBNode));
    freeBlocks.insert(root);
}

Byte* FreeListAllocator::allocate(USize bytes, USize alignment) noexcept
{
    assert(bytes > USize(0) && "Invalid allocation!");
    alignment = Memory::align_binary_safe(alignment);
    bytes += (sizeof(Void *) - (bytes & (sizeof(Void *) - 1))) & (sizeof(Void *) - 1);
    RBNode *data;
    if (alignment == sizeof(Void*)) [[likely]]
    {
        data = freeBlocks.find(bytes);
    } else {
        data = freeBlocks.find(bytes + alignment - USize(1));
    }

    freeBlocks.remove(data);
    data = freeBlocks.split_node(data, bytes, alignment);
    return data->get_memory();
}

Void FreeListAllocator::deallocate(Byte* pointer) noexcept
{
    assert(pointer != nullptr && "Null pointer cannot be deallocated!");
    assert(memory + capacity > pointer && "Pointer out of scope!");
    RBNode *node = reinterpret_cast<RBNode *>(pointer - sizeof(RBNode));
    freeBlocks.insert(node);
}

Void FreeListAllocator::copy(const FreeListAllocator& source) noexcept
{
    assert(this != &source && "Attempted to copy allocator into itself!");
    assert(source.memory != nullptr && "Copying from an empty allocator. Destination will also be empty.");

    finalize();
    if (!source.parentInfo)
    {
        initialize(source.capacity - sizeof(RBNode));
    } else {
        initialize(source.capacity - sizeof(RBNode), source.parentInfo);
    }

}

Void FreeListAllocator::move(FreeListAllocator& source) noexcept
{
    assert(this != &source && "Attempted to move allocator into itself!");

    finalize();
    parentInfo = source.parentInfo;
    freeBlocks = source.freeBlocks;
    memory     = source.memory;
    capacity   = source.capacity;

    source = {};
}

Void FreeListAllocator::print_list() const noexcept
{
    RBNode *node = reinterpret_cast<RBNode*>(memory);
    while (node)
    {
        printf("%llu(%s)->", node->get_size(), node->is_free() ? "free" : "reserved");
        node = node->get_next();
    }
    printf("\n");
}

USize FreeListAllocator::get_capacity() const noexcept
{
    return capacity;
}

Void FreeListAllocator::finalize() noexcept
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

AllocatorInfo *FreeListAllocator::get_allocator_info() noexcept
{
    return &selfInfo;
}