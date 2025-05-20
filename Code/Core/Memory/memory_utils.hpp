#pragma once
#include "byte.hpp"

struct AllocatorInfo
{
    using Allocate   = Byte *(*)(Void *allocator, USize bytes, USize alignment);
    using Deallocate = Void(*)(Void *allocator, Byte *pointer);

    Void *allocator;
    Allocate allocate;
    Deallocate deallocate;

    static AllocatorInfo *get_default_allocator()
    {
        static AllocatorInfo defaultAllocator = 
        {
            nullptr,
            [](Void *allocator, USize bytes, USize alignment) -> Byte *{ return byte_cast(_aligned_malloc(bytes, alignment)); },
            [](Void *allocator, Byte *pointer) { free(pointer); }
        };
        return &defaultAllocator;
    }
};
