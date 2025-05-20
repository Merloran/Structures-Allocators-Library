#include "pool_allocator.hpp"

Void PoolAllocator::initialize(const USize count, const USize size) noexcept
{
    blockSize = size;
    capacity = align_memory(count * size);
    memory = byte_cast(VirtualAlloc(nullptr, 
                                           capacity, 
                                           MEM_RESERVE | MEM_COMMIT, 
                                           PAGE_READWRITE));

    assert(memory != nullptr && "Allocation failed!");

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, USize bytes, USize alignment) -> Byte *
    {
        return static_cast<PoolAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<PoolAllocator *>(allocator)->deallocate(pointer);
    };

    freeList = new (memory) PoolBlock();
    PoolBlock *current = freeList;
    for (USize i = 0; i < count; ++i)
    {
        current->next = new (memory + i * blockSize) PoolBlock();
        current = current->next;
    }
    current->next = nullptr;
}

Void PoolAllocator::initialize(const USize count, const USize size, AllocatorInfo *allocatorInfo) noexcept
{
    blockSize = size;
    parentInfo = allocatorInfo;

    assert(parentInfo != nullptr && "Parent allocator is nullptr!");

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, USize bytes, USize alignment) -> Byte *
    {
        return static_cast<PoolAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<PoolAllocator *>(allocator)->deallocate(pointer);
    };

    capacity = count * size;
    memory = parentInfo->allocate(parentInfo->allocator, capacity, alignof(PoolBlock));

    freeList = new (memory) PoolBlock();
    PoolBlock *current = freeList;
    for (USize i = 0; i < count; ++i)
    {
        current->next = new (memory + i * blockSize) PoolBlock();
        current = current->next;
    }
    current->next = nullptr;
}

Byte* PoolAllocator::allocate(const USize bytes, [[maybe_unused]] const USize alignment) noexcept
{
    assert(bytes <= blockSize && "Requested too much memory!");
    assert(freeList != nullptr && "Out of memory!");

    Byte *address = byte_cast(freeList);
    freeList = freeList->next;
    return address;
}

Void PoolAllocator::deallocate(Byte *pointer) noexcept
{
    const USize offset = USize(pointer) - USize(memory);
    assert(offset < capacity && "Pointer out of scope!");

    pointer -= offset % blockSize;

    PoolBlock *freeBlock = new (pointer) PoolBlock();
    freeBlock->next = freeList;
    freeList = freeBlock;
}

Void PoolAllocator::copy(const PoolAllocator& source) noexcept
{
    assert(this != &source && "Attempted to copy allocator into itself!");
    assert(source.memory != nullptr && "Copying from an empty allocator. Destination will also be empty.");

    finalize();
    if (!source.parentInfo)
    {
        initialize(source.capacity / blockSize, blockSize);
    } else {
        initialize(source.capacity / blockSize, blockSize, source.parentInfo);
    }
}

Void PoolAllocator::move(PoolAllocator& source) noexcept
{
    assert(this != &source && "Attempted to move allocator into itself!");

    finalize();
    parentInfo    = source.parentInfo;
    memory        = source.memory;
    freeList      = source.freeList;
    capacity      = source.capacity;
    blockSize     = source.blockSize;
    source = {};
}

Void PoolAllocator::finalize() noexcept
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

AllocatorInfo *PoolAllocator::get_allocator_info() noexcept
{
    return &selfInfo;
}