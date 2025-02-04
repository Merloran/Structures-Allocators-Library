#include "Core/Structures/rb_node.hpp"
#include "Memory/memory.hpp"
Int32 main()
{
    Void *memory = malloc(5_GB);
    RBNode node{};
    node.set_color(RBNode::EColor::Red);
    SPDLOG_INFO(node.get_color() == RBNode::EColor::Black ? "BLACK" : "RED");
    node.set_free(false);
    SPDLOG_INFO(node.is_free() ? "FREE" : "NOPE");
    UInt64 offset = 4500_MB;
    SPDLOG_INFO("original {}", static_cast<const void *>(reinterpret_cast<UInt8 *>(memory) + offset));

    node.set_parent(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("parent {}", static_cast<const void *>(node.get_parent(memory)));
    node.set_left(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("left {}", static_cast<const void*>(node.get_left(memory)));
    SPDLOG_INFO("parent {}", static_cast<const void *>(node.get_parent(memory)));
    node.set_parent(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("parent {}", static_cast<const void *>(node.get_parent(memory)));
    SPDLOG_INFO("left {}", static_cast<const void *>(node.get_left(memory)));
    node.set_right(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("right {}", static_cast<const void *>(node.get_right(memory)));
    node.set_next(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("next {}", static_cast<const void *>(node.get_next(memory)));
    node.set_previous(reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + offset), memory);
    SPDLOG_INFO("previous {}", static_cast<const void *>(node.get_previous(memory)));
    node.set_size(812012);
    SPDLOG_INFO(node.get_size());

    free(memory);
    return 0;
}