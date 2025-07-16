#include "rb_node_packed.hpp"

#include <cassert>

RBNodePacked *RBNodePacked::get_parent(Byte *memory) const noexcept
{
    constexpr USize DATA_OFFSET = 0;
    USize result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNodePacked *address = reinterpret_cast<RBNodePacked *>(memory + (result & 0xFFFFFFFFF));
    return byte_cast(address) == memory ? nullptr : address;
}

RBNodePacked *RBNodePacked::get_left(Byte *memory) const noexcept
{
    constexpr USize DATA_OFFSET = 4;
    USize result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNodePacked *address = reinterpret_cast<RBNodePacked *>(memory + ((result & 0xFFFFFFFFFF) >> 4));
    return byte_cast(address) == memory ? nullptr : address;
}

RBNodePacked *RBNodePacked::get_right(Byte *memory) const noexcept
{
    constexpr USize DATA_OFFSET = 9;
    USize result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNodePacked *address = reinterpret_cast<RBNodePacked *>(memory + (result & 0xFFFFFFFFF));
    return byte_cast(address) == memory ? nullptr : address;
}

RBNodePacked *RBNodePacked::get_next() const noexcept
{
    const Bool isNextSet = Bool(data[22] & 0x20_B);
    if (isNextSet)
    {
        const USize exactNodeSize = sizeof(RBNodePacked) + get_size();
        return reinterpret_cast<RBNodePacked*>(byte_cast(const_cast<RBNodePacked *>(this)) + exactNodeSize);
    }
    return nullptr;
}

RBNodePacked *RBNodePacked::get_previous(Byte *memory) const noexcept
{
    constexpr USize DATA_OFFSET = 13;
    USize result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    RBNodePacked *address = reinterpret_cast<RBNodePacked *>(memory + ((result & 0xFFFFFFFFFF) >> 4));
    return byte_cast(address) == memory ? nullptr : address;
}

USize RBNodePacked::get_size() const noexcept
{
    constexpr USize DATA_OFFSET = 18;
    USize result = 0;
    memcpy(&result, data + DATA_OFFSET, 5);
    return result & 0xFFFFFFFFF;
}

RBNodePacked::EColor RBNodePacked::get_color() const noexcept
{
    constexpr USize DATA_OFFSET = 22;
    const Bool result = Bool(data[DATA_OFFSET] & 0x80_B);
    return result ? EColor::Red : EColor::Black;
}

Bool RBNodePacked::is_free() const noexcept
{
    constexpr USize DATA_OFFSET = 22;
    const Bool result = Bool(data[DATA_OFFSET] & 0x40_B);
    return !result;
}

Byte *RBNodePacked::get_memory() const noexcept
{
    return byte_cast(const_cast<RBNodePacked *>(this)) + sizeof(RBNodePacked);
}

Void RBNodePacked::set_parent(RBNodePacked* parent, const Byte* memory) noexcept
{
    constexpr USize DATA_OFFSET = 0;
    USize offset = 0;
    if (parent)
    {
        offset = USize(byte_cast(parent)) - USize(memory);
    }
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &offset, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xF0_B;
    data[DATA_OFFSET + 4] |= Byte(offset >> 32) & 0x0F_B;
}

Void RBNodePacked::set_left(RBNodePacked* left, const Byte* memory) noexcept
{
    constexpr USize DATA_OFFSET = 4;

    USize offset = 0;
    if (left)
    {
        offset = USize(byte_cast(left)) - USize(memory);
        offset <<= 4;
    }
    // Set last 4 bytes to data
    memcpy(data + DATA_OFFSET + 1, byte_cast(&offset) + 1, 4);
    // Set second half of first shared byte
    data[DATA_OFFSET] &= 0x0F_B;
    data[DATA_OFFSET] |= Byte(offset) & 0xF0_B;
}

Void RBNodePacked::set_right(RBNodePacked* right, const Byte* memory) noexcept
{
    constexpr USize DATA_OFFSET = 9;
    USize offset = 0;
    if (right)
    {
        offset = USize(byte_cast(right)) - USize(memory);
    }
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &offset, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xF0_B;
    data[DATA_OFFSET + 4] |= Byte(offset >> 32) & 0x0F_B;
}

Void RBNodePacked::set_next(const RBNodePacked* next) noexcept
{
    constexpr USize DATA_OFFSET = 22;
    if (next)
    {
        data[DATA_OFFSET] |= 0x20_B;
    } else {
        data[DATA_OFFSET] &= ~0x20_B;
    }
}

Void RBNodePacked::set_previous(RBNodePacked* previous, const Byte* memory) noexcept
{
    constexpr USize DATA_OFFSET = 13;

    USize offset = 0;
    if (previous)
    {
        offset = USize(byte_cast(previous)) - USize(memory);
        offset <<= 4;
    }
    // Set last 4 bytes to data
    memcpy(data + DATA_OFFSET + 1, byte_cast(&offset) + 1, 4);
    // Set second half of first shared byte
    data[DATA_OFFSET] &= 0x0F_B;
    data[DATA_OFFSET] |= Byte(offset) & 0xF0_B;
}

Void RBNodePacked::set_size(const USize size) noexcept
{
    assert(size <= MAX_NODE_SIZE && "Node size is too large!");
    constexpr USize DATA_OFFSET = 18;
    // Set first 4 bytes to data
    memcpy(data + DATA_OFFSET, &size, 4);
    // Set first half of last shared byte
    data[DATA_OFFSET + 4] &= 0xF0_B;
    data[DATA_OFFSET + 4] |= Byte(size >> 32) & 0x0F_B;
}

Void RBNodePacked::set_color(const EColor color) noexcept
{
    constexpr USize DATA_OFFSET = 22;
    if (color == EColor::Red)
    {
        data[DATA_OFFSET] |= 0x80_B;
    } else {
        data[DATA_OFFSET] &= ~0x80_B;
    }
}

Void RBNodePacked::set_free(const Bool isFree) noexcept
{
    constexpr USize DATA_OFFSET = 22;
    if (!isFree)
    {
        data[DATA_OFFSET] |= 0x40_B;
    } else {
        data[DATA_OFFSET] &= ~0x40_B;
    }
}

Void RBNodePacked::reset() noexcept
{
    //Set parent, left and right to nullptr
    memset(data, 0, 13);
    data[13] &= 0xF0_B;

    set_color(EColor::Red);
    set_free(true);
}