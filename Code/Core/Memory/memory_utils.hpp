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
            [](Void *allocator, Byte *pointer) { _aligned_free(pointer); }
        };
        return &defaultAllocator;
    }
};

namespace Memory
{
    constexpr USize align_binary_safe(const USize value) noexcept
    {
        if (value <= USize(8))
        {
            return USize(8);
        }
        return USize(1) << std::bit_width(value - USize(1));
    }

    template<Manual Type, Bool CallConstructor = true>
    Type *start_object(Byte *memory) noexcept
    {
        assert(memory && "Invalid pointer!");

        if constexpr (CallConstructor)
        {
            return std::launder(new (memory) Type{});
        } else {
            return std::launder(static_cast<Type *>(std::memmove(memory, memory, sizeof(Type))));
        }

    }

    template<Manual Type, Bool CallConstructors = true>
    Type *start_object(Byte *memory, const USize count) noexcept
    {
        assert(memory && "Invalid pointer!");
        assert(count > 1 && "Count should be bigger than 1 or not be passed as parameter!");

        if constexpr (CallConstructors)
        {
            Type *startedArray = reinterpret_cast<Type *>(memory);
            for (USize i = 0; i < count; ++i)
            {
                new (startedArray + i) Type{};
            }
            return std::launder(startedArray);
        } else {
            return std::launder(static_cast<Type *>(std::memmove(memory, memory, sizeof(Type) * count)));
        }
    }

    template <Manual Type, Bool CallConstructor = true>
    Type *allocate(AllocatorInfo *allocatorInfo) noexcept
    {
        assert(allocatorInfo && "Invalid pointer!");
        return start_object<Type, CallConstructor>(allocatorInfo->allocate(allocatorInfo->allocator, 
                                                                                 sizeof(Type), 
                                                                                 alignof(Type)));
    }

    template <Manual Type, Bool CallConstructor = true>
    Type *allocate(AllocatorInfo *allocatorInfo, const USize count) noexcept
    {
        assert(allocatorInfo && "Invalid pointer!");
        assert(count > 1 && "Count should be bigger than 1 or not be passed as parameter!");
        return start_object<Type, CallConstructor>(allocatorInfo->allocate(allocatorInfo->allocator, 
                                                                                 count * sizeof(Type), 
                                                                                 alignof(Type)),
                                                   count);
    }

    template <Manual Type>
    Void deallocate(AllocatorInfo *allocatorInfo, Type *element)
    {
        allocatorInfo->deallocate(allocatorInfo->allocator, byte_cast(element));
    }

}