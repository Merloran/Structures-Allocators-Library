#include "Memory/memory_utils.hpp"
#include "Memory/freelist_allocator.hpp"
#include "Memory/pool_allocator.hpp"
#include "Memory/stack_allocator.hpp"

Int32 main()
{
    FreeListAllocator global;
    global.initialize(10_KiB);
    PoolAllocator sub;
    sub.initialize(10, 128_B, global.get_allocator_info());
    StackAllocator local;
    local.initialize(64_B, sub.get_allocator_info());
    local.finalize();
    sub.finalize();
    global.finalize();
    return 0;
}
