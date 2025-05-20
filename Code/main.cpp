#include <iostream>
#include <chrono>
#include <string>
#include <algorithm>

#include "Structures/string.hpp"
#include "Memory/freelist_allocator.hpp"
#include "Memory/pool_allocator.hpp"
#include "Memory/stack_allocator.hpp"
#include "Structures/dynamic_array.hpp"

int main() 
{
    StackAllocator aaa{};
    aaa.initialize(8_KiB);
    DynamicArray<Float32> bbb{};
    bbb.initialize(10, aaa.get_allocator_info());

    aaa.finalize();
    return 0;
}
