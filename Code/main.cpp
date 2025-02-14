#include "Memory/memory.hpp"
#include "Memory/freelist_allocator.hpp"

Int32 main()
{
    FreeListAllocator allocator;
    allocator.initialize(10_KiB);

    SPDLOG_INFO("Allocate 300");
    Void *rawData2 = allocator.allocate(300_B);
    SPDLOG_INFO("Allocate 128");
    Void *rawData = allocator.allocate(128_B);
    SPDLOG_INFO("Allocate 80");
    UInt64 *data = allocator.allocate<UInt64>(10);
    SPDLOG_INFO("Allocate 250");
    Void *rawData1 = allocator.allocate(250_B);
    RBNode *node = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(rawData2) - sizeof(RBNode));
    for (UInt64 i = 0; i < 5; ++i)
    {
        std::cout << node->get_size() << "->";
        node = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(node) + node->get_size() + sizeof(RBNode));
    }
    std::cout << std::endl;
    //(mem) -> r1 -> d -> r -> r2
    //               ^
    //(mem) -> r1 -> (d) -> r -> r2
    //                           ^
    //(mem) -> r1 -> (d) -> r -> (r2)
    //         ^
    //(mem) -> r -> (r2)
    SPDLOG_INFO("Deallocate 80");
    allocator.deallocate(data);
    SPDLOG_INFO("Deallocate 300");
    allocator.deallocate(rawData2);
    SPDLOG_INFO("Deallocate 250");
    allocator.deallocate(rawData1);
    SPDLOG_INFO("Deallocate 128");
    allocator.deallocate(rawData);

    allocator.finalize();

    return 0;
}