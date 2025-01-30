#include "Core/Memory/stack_allocator.hpp"
#include "Core/Memory/pool_allocator.hpp"
Int32 main()
{
    PoolAllocator allocator;
    StackAllocator allocator2;
    allocator.initialize(10, 4000);
    UInt64 *ptr = allocator.allocate<UInt64>(10);
    allocator2.initialize(1000, allocator.get_allocator_info());

    SPDLOG_INFO(*ptr);
    allocator2.finalize();
    allocator.finalize();

    return 0;
}