#pragma once

//Node contains compressed pointers to 4.5 bytes
struct RBNodePacked
{
public:
    enum class EColor : UInt8
    {
        Red,
        Black
    };

private:
    static constexpr USize MAX_NODE_SIZE = 0xfffffffff; // 2 ^ 36, about 64 GB
    Byte data[24]; // TODO: Maybe change it to bitfields?
    // 4.5B (parent) | 4.5B (left) | 4.5B (right) | 4.5B (previous) | 4.5B (size)
    // | 1b (color) | 1b (isFree) | 1b (isNextSet) | 9b (unused)

public:
    RBNodePacked() noexcept
    {
        static_assert(sizeof(*this) % sizeof(Void *) == 0);
        static_assert(alignof(RBNodePacked) <= 8);
        memset(data, 0, 24);
    }

    [[nodiscard]]
    RBNodePacked *get_parent(Byte *memory) const noexcept;
    [[nodiscard]]
    RBNodePacked *get_left(Byte *memory) const noexcept;
    [[nodiscard]]
    RBNodePacked *get_right(Byte *memory) const noexcept;
    // It calculates pointer based on size
    [[nodiscard]]
    RBNodePacked *get_next() const noexcept;
    [[nodiscard]]
    RBNodePacked *get_previous(Byte *memory) const noexcept;
    [[nodiscard]]
    USize  get_size() const noexcept;
    [[nodiscard]]
    EColor get_color() const noexcept;
    [[nodiscard]]
    Bool   is_free() const noexcept;
    [[nodiscard]]
    Byte  *get_memory() const noexcept;

    Void set_parent(RBNodePacked *parent, const Byte *memory) noexcept;
    Void set_left(RBNodePacked *left, const Byte *memory) noexcept;
    Void set_right(RBNodePacked *right, const Byte *memory) noexcept;
    // It sets isNextSet flag, false for nullptr, so remember to get_next before change size, also see get_next
    Void set_next(const RBNodePacked *next) noexcept;
    Void set_previous(RBNodePacked *previous, const Byte *memory) noexcept;
    Void set_size(USize size) noexcept;
    Void set_color(EColor color) noexcept;
    Void set_free(Bool isFree) noexcept;

    Void reset() noexcept;
};