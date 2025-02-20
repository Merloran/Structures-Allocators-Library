#include <iostream>

#include "Memory/freelist_allocator.hpp"
#include "Structures/dynamic_array.hpp"

Int32 main()
{
    FreeListAllocator allocator;
    allocator.initialize(100_MiB);
    
    DynamicArray<Float32> floats;
    floats.initialize(1000, 3.0f, allocator.get_allocator_info());

    for (auto a : floats)
    {
        std::cout << a << "\n";
    }

    for (UInt64 i = 0; i < 10000; ++i)
    {
        floats.append(10.0f);
    }

    floats.finalize();
    allocator.finalize();

    return 0;
}