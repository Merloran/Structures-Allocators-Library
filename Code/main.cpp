#include <iostream>

#include "Memory/freelist_allocator.hpp"
#include "Structures/dynamic_array.hpp"
#include "Structures/string.hpp"

#define TEST_SIZE 1000000  // Liczba elementów w teście

#define TIME_BLOCK(name, code) \
    { \
        auto start = std::chrono::high_resolution_clock::now(); \
        code \
        auto end = std::chrono::high_resolution_clock::now(); \
        std::cout << name << ": " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n"; \
    }

Int32 main()
{
    FreeListAllocator allocator;
    allocator.initialize(10_MiB);
    String32 str1;
    str1.initialize(U"Hello", allocator.get_allocator_info());
    String str2;
    str2.initialize("World", allocator.get_allocator_info());

    std::wcout << str1;

    str1.finalize();
    str2.finalize();
    allocator.finalize();
    return 0;
}