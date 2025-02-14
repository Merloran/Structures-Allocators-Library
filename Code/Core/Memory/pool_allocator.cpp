#include "pool_allocator.hpp"

Void PoolAllocator::initialize(const UInt64 count, const UInt64 size)
{
    blockSize = size;
    capacity = align_memory(count * size);
    memory = VirtualAlloc(nullptr, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory)
    {
        SPDLOG_CRITICAL("Allocation failed!");
        assert(false);
    }

    freeList = static_cast<PoolBlock *>(memory);
    PoolBlock *current = freeList;
    for (UInt64 i = 0; i < count; ++i)
    {
        current->next = reinterpret_cast<PoolBlock *>(reinterpret_cast<UInt8 *>(memory) + i * blockSize);
        current = current->next;
    }
    current->next = nullptr;

    isIndependent = true;
}

Void PoolAllocator::initialize(const UInt64 count, const UInt64 size, const AllocatorInfo& allocatorInfo)
{
    blockSize = size;
    parentInfo = allocatorInfo;

    if (!parentInfo.allocator)
    {
        SPDLOG_WARN("Parent allocator is nullptr!");
        return;
    }

    capacity = count * size;
    memory = parentInfo.allocate(parentInfo.allocator, capacity);

    freeList = static_cast<PoolBlock *>(memory);
    PoolBlock *current = freeList;
    for (UInt64 i = 0; i < count; ++i)
    {
        current->next = reinterpret_cast<PoolBlock *>(reinterpret_cast<UInt8 *>(memory) + i * blockSize);
        current = current->next;
    }
    current->next = nullptr;

    isIndependent = false;
}

Void* PoolAllocator::allocate(const UInt64 bytes)
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

Void PoolAllocator::deallocate(Void* pointer)
{
    const UInt64 offset = reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory);
    if (offset >= capacity)
    {
        SPDLOG_WARN("Pointer out of scope!");
        return;
    }

    UInt64 pointerAlignment = (reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory)) % blockSize;
    if (pointerAlignment != 0)
    {
        pointer = reinterpret_cast<Void *>(reinterpret_cast<UInt8 *>(pointer) - pointerAlignment);
    }

    PoolBlock *freeBlock = static_cast<PoolBlock *>(pointer);
    freeBlock->next = freeList;
    freeList = freeBlock;
}

Void PoolAllocator::copy(const PoolAllocator& source)
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

    finalize();
    if (source.isIndependent)
    {
        initialize(source.capacity / blockSize, blockSize);
    } else {
        initialize(source.capacity / blockSize, blockSize, source.parentInfo);
    }
}

Void PoolAllocator::move(PoolAllocator& source)
{
    if (this == &source)
    {
        SPDLOG_WARN("Attempted to move allocator into itself. Skipping.");
        return;
    }
    finalize();
    parentInfo    = source.parentInfo;
    memory        = source.memory;
    freeList      = source.freeList;
    capacity      = source.capacity;
    blockSize     = source.blockSize;
    isIndependent = source.isIndependent;
    source = {};
}

Void PoolAllocator::finalize()
{
    if (!memory)
    {
        *this = {};
        return;
    }

    if (isIndependent)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    } else {
        parentInfo.deallocate(parentInfo.allocator, memory);
    }
    *this = {};
}

AllocatorInfo PoolAllocator::get_allocator_info()
{
    AllocatorInfo info;
    info.allocator = this;
    info.allocate = [](Void *allocator, UInt64 bytes) -> Void *
    {
        return static_cast<PoolAllocator *>(allocator)->allocate(bytes);
    };

    info.deallocate = [](Void *allocator, Void *pointer) -> Void
    {
        static_cast<PoolAllocator *>(allocator)->deallocate(pointer);
    };
    return info;
}