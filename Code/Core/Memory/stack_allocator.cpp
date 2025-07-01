#include "stack_allocator.hpp"

Void StackAllocator::initialize(const USize bytes) noexcept
{
    offset = 0;
    capacity = align_system_memory(bytes);
    memory = byte_cast(VirtualAlloc(nullptr, 
                                           capacity, 
                                           MEM_RESERVE | MEM_COMMIT, 
                                           PAGE_READWRITE));
    assert(memory != nullptr && "Allocation failed!");

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, USize bytes, USize alignment) -> Byte *
    {
        return static_cast<StackAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<StackAllocator *>(allocator)->deallocate(pointer);
    };
}

Void StackAllocator::initialize(const USize bytes, AllocatorInfo *allocatorInfo) noexcept
{
    offset = 0;
    parentInfo = allocatorInfo;

    assert(parentInfo != nullptr && "Parent allocator is nullptr!");

    selfInfo.allocator = this;
    selfInfo.allocate = [](Void *allocator, USize bytes, USize alignment) -> Byte *
    {
        return static_cast<StackAllocator *>(allocator)->allocate(bytes, alignment);
    };

    selfInfo.deallocate = [](Void *allocator, Byte *pointer) -> Void
    {
        static_cast<StackAllocator *>(allocator)->deallocate(pointer);
    };

    capacity = bytes;
    memory = parentInfo->allocate(parentInfo->allocator, capacity, alignof(USize));
}

Byte* StackAllocator::allocate(const USize bytes, const USize alignment) noexcept
{
    const USize address = USize(memory + offset);
    const USize padding = alignment - (address % alignment);

    offset += bytes + padding;
    assert(offset <= capacity && "Out of memory!");
    return byte_cast(address + padding);
}

Void StackAllocator::deallocate(const USize marker) noexcept
{
    if (marker <= offset)
    {
        offset = marker;
    }
}

Void StackAllocator::deallocate(Byte* pointer) noexcept
{
    USize marker = USize(pointer) - USize(memory);
    if (marker <= offset)
    {
        offset = marker;
    }
}

Void StackAllocator::copy(const StackAllocator& source) noexcept
{
    assert(this != &source && "Attempted to copy allocator into itself!");
    assert(source.memory != nullptr && "Copying from an empty allocator. Destination will also be empty.");
    finalize();

    if (!source.parentInfo)
    {
        initialize(source.capacity);
    } else {
        initialize(source.capacity, source.parentInfo);
    }
}

Void StackAllocator::move(StackAllocator& source) noexcept
{
    assert(this != &source && "Attempted to move allocator into itself!");

    finalize();
    parentInfo = source.parentInfo;
    memory = source.memory;
    capacity = source.capacity;
    offset = source.offset;
    source = {};
}

Void StackAllocator::finalize() noexcept
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

AllocatorInfo *StackAllocator::get_allocator_info() noexcept
{
    return &selfInfo;
}