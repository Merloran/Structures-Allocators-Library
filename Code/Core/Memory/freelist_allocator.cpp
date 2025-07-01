#include "freelist_allocator.hpp"

#include "Structures/rb_node_packed.hpp"


Void FreeListAllocator::initialize(const USize bytes) noexcept
{
    capacity = align_system_memory(bytes + sizeof(RBNodePacked));
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
    RBNodePacked *root = Memory::start_object<RBNodePacked>(memory);
    root->set_size(capacity - sizeof(RBNodePacked));
    freeBlocks.insert(root);
    print_list();
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

    capacity = bytes + sizeof(RBNodePacked);
    memory = parentInfo->allocate(parentInfo->allocator, capacity, alignof(RBNodePacked));
    freeBlocks = { memory };
    RBNodePacked *root = Memory::start_object<RBNodePacked>(memory);
    root->set_size(capacity - sizeof(RBNodePacked));
    freeBlocks.insert(root);
    print_list();
}

Byte* FreeListAllocator::allocate(USize bytes, USize alignment) noexcept
{
    assert(bytes > USize(0) && "Invalid allocation!");
    alignment = Memory::align_binary_safe(alignment);
    bytes += (sizeof(Void *) - (bytes & (sizeof(Void *) - 1))) & (sizeof(Void *) - 1);
    RBNodePacked *data = freeBlocks.find(bytes + alignment - USize(1));
    freeBlocks.remove(data);
    data = freeBlocks.split_node(data, bytes, alignment);
    return data->get_memory();
}

Void FreeListAllocator::deallocate(Byte* pointer) noexcept
{
    assert(pointer != nullptr && "Null pointer cannot be deallocated!");
    assert(memory + capacity > pointer && "Pointer out of scope!");
    RBNodePacked *node = reinterpret_cast<RBNodePacked *>(pointer - sizeof(RBNodePacked));
    freeBlocks.insert(node);
}

Void FreeListAllocator::copy(const FreeListAllocator& source) noexcept
{
    assert(this != &source && "Attempted to copy allocator into itself!");
    assert(source.memory != nullptr && "Copying from an empty allocator. Destination will also be empty.");

    finalize();
    if (!source.parentInfo)
    {
        initialize(source.capacity - sizeof(RBNodePacked));
    } else {
        initialize(source.capacity - sizeof(RBNodePacked), source.parentInfo);
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

Void FreeListAllocator::print_list() noexcept
{
    RBNodePacked *node = reinterpret_cast<RBNodePacked*>(memory);
    while (node)
    {
        printf("%llu(%s)->", node->get_size(), node->is_free() ? "free" : "reserved");
        node = node->get_next();
    }
    printf("\n");
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