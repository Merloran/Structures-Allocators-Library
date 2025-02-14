#include "stack_allocator.hpp"

Void StackAllocator::initialize(const UInt64 bytes)
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

Void StackAllocator::initialize(const UInt64 bytes, const AllocatorInfo& allocatorInfo)
{
    offset = 0;
    parentInfo = allocatorInfo;

    if (!parentInfo.allocator)
    {
        SPDLOG_WARN("Parent allocator is nullptr!");
        return;
    }

    capacity = bytes;
    memory = parentInfo.allocate(parentInfo.allocator, capacity);
    isIndependent = false;
}

Void* StackAllocator::allocate(const UInt64 bytes)
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

Void StackAllocator::deallocate(const UInt64 marker)
{
    if (marker <= offset)
    {
        offset = marker;
    }
}

Void StackAllocator::deallocate(Void* pointer)
{
    UInt64 marker = reinterpret_cast<UInt8 *>(pointer) - reinterpret_cast<UInt8 *>(memory);
    if (marker <= offset)
    {
        offset = marker;
    }
}

Void StackAllocator::copy(const StackAllocator& source)
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
        initialize(source.capacity);
    } else {
        initialize(source.capacity, source.parentInfo);
    }
}

Void StackAllocator::move(StackAllocator& source)
{
    if (this == &source)
    {
        SPDLOG_WARN("Attempted to move allocator into itself. Skipping.");
        return;
    }
    finalize();
    parentInfo = source.parentInfo;
    memory = source.memory;
    capacity = source.capacity;
    offset = source.offset;
    isIndependent = source.isIndependent;
    source = {};
}

Void StackAllocator::finalize()
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

AllocatorInfo StackAllocator::get_allocator_info()
{
    AllocatorInfo info;
    info.allocator = this;
    info.allocate = [](Void *allocator, UInt64 bytes) -> Void *
    {
        return static_cast<StackAllocator *>(allocator)->allocate(bytes);
    };

    info.deallocate = [](Void *allocator, Void *pointer) -> Void
    {
        static_cast<StackAllocator *>(allocator)->deallocate(pointer);
    };
    return info;
}