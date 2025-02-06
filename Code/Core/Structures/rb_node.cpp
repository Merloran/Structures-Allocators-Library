#include "rb_node.hpp"

#include "Memory/memory.hpp"

RBNode *RBNode::get_parent(Void *memory) const
{
    constexpr UInt64 DATA_OFFSET = 0_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNode *address = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + (result & 0xfffffffff));
    return address == memory ? nullptr : address;
}

RBNode *RBNode::get_left(Void *memory) const
{
    constexpr UInt64 DATA_OFFSET = 4_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNode *address = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(memory) + ((result & 0xffffffffff) >> 4));
    return address == memory ? nullptr : address;
}

RBNode *RBNode::get_right(Void *memory) const
{
    constexpr UInt64 DATA_OFFSET = 9_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNode *address = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + (result & 0xfffffffff));
    return address == memory ? nullptr : address;
}

RBNode *RBNode::get_next(Void *memory) const
{
    constexpr UInt64 DATA_OFFSET = 13_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNode *address = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + ((result & 0xffffffffff) >> 4));
    return address == memory ? nullptr : address;
}

RBNode *RBNode::get_previous(Void *memory) const
{
    constexpr UInt64 DATA_OFFSET = 18_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNode *address = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(memory) + (result & 0xfffffffff));
    return address == memory ? nullptr : address;
}

UInt64 RBNode::get_size() const
{
    constexpr UInt64 DATA_OFFSET = 22_B;
    UInt64 result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    return (result & 0xffffffffff) >> 4;
}

RBNode::EColor RBNode::get_color() const
{
    constexpr UInt64 DATA_OFFSET = 28_B;
    const Bool result = data[DATA_OFFSET] & 0x80;
    return result ? EColor::Red : EColor::Black;
}

Bool RBNode::is_free() const
{
    constexpr UInt64 DATA_OFFSET = 28_B;
    const Bool result = data[DATA_OFFSET] & 0x40;
    return !result;
}

Void* RBNode::get_memory()
{
    return reinterpret_cast<UInt8*>(this) + sizeof(RBNode);
}

Void RBNode::set_parent(RBNode* parent, Void* memory)
{
    constexpr UInt64 DATA_OFFSET = 0_B;
    UInt64 offset = 0;
    if (parent)
    {
        offset = reinterpret_cast<UInt8 *>(parent) - reinterpret_cast<UInt8 *>(memory);
    }
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &offset, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xf0;
    data[DATA_OFFSET + 4] |= offset >> 32 & 0x0f;
}

Void RBNode::set_left(RBNode* left, Void* memory)
{
    constexpr UInt64 DATA_OFFSET = 4_B;

    UInt64 offset = 0;
    if (left)
    {
        offset = reinterpret_cast<UInt8 *>(left) - reinterpret_cast<UInt8 *>(memory);
        offset <<= 4;
    }
    // Set last 4 bytes to data
    memcpy(data + DATA_OFFSET + 1, reinterpret_cast<UInt8 *>(&offset) + 1, 4);
    // Set second half of first shared byte
    data[DATA_OFFSET] &= 0x0f;
    data[DATA_OFFSET] |= offset & 0xf0;
}

Void RBNode::set_right(RBNode* right, Void* memory)
{
    constexpr UInt64 DATA_OFFSET = 9_B;
    UInt64 offset = 0;
    if (right)
    {
        offset = reinterpret_cast<UInt8 *>(right) - reinterpret_cast<UInt8 *>(memory);
    }
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &offset, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xf0;
    data[DATA_OFFSET + 4] |= offset >> 32 & 0x0f;
}

Void RBNode::set_next(RBNode* next, Void* memory)
{
    constexpr UInt64 DATA_OFFSET = 13_B;

    UInt64 offset = 0;
    if (next)
    {
        offset = reinterpret_cast<UInt8 *>(next) - reinterpret_cast<UInt8 *>(memory);
        offset <<= 4;
    }
    // Set last 4 bytes to data
    memcpy(data + DATA_OFFSET + 1, reinterpret_cast<UInt8 *>(&offset) + 1, 4);
    // Set second half of first shared byte
    data[DATA_OFFSET] &= 0x0f;
    data[DATA_OFFSET] |= offset & 0xf0;
}

Void RBNode::set_previous(RBNode* previous, Void* memory)
{
    constexpr UInt64 DATA_OFFSET = 18_B;
    UInt64 offset = 0;
    if (previous)
    {
        offset = reinterpret_cast<UInt8 *>(previous) - reinterpret_cast<UInt8 *>(memory);
    }
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &offset, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xf0;
    data[DATA_OFFSET + 4] |= offset >> 32 & 0x0f;
}

Void RBNode::set_size(UInt64 size)
{
    if (size > MAX_NODE_SIZE)
    {
        SPDLOG_CRITICAL("Node size is to large: {}!!!", size);
        assert(false);
        return;
    }
    constexpr UInt64 DATA_OFFSET = 22_B;
    size <<= 4;
    // Set last 4 bytes to data
    memcpy(data + DATA_OFFSET + 1, reinterpret_cast<UInt8 *>(&size) + 1, 4);
    // Set second half of first shared byte
    data[DATA_OFFSET] &= 0xf0;
    data[DATA_OFFSET] |= size & 0xf0;
}

Void RBNode::set_color(const EColor color)
{
    constexpr UInt64 DATA_OFFSET = 28_B;
    if (color == EColor::Red)
    {
        data[DATA_OFFSET] |= 0x80;
    } else {
        data[DATA_OFFSET] &= ~0x80;
    }
}

Void RBNode::set_free(const Bool isFree)
{
    constexpr UInt64 DATA_OFFSET = 28_B;
    if (!isFree)
    {
        data[DATA_OFFSET] |= 0x40;
    } else {
        data[DATA_OFFSET] &= ~0x40;
    }
}
