#include <iostream>
#include <chrono>
#include <string>
#include <algorithm>
#include <memory>

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
    std::list<int> ccc;
    aaa.finalize();
    return 0;
}
