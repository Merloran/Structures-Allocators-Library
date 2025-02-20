#pragma once
//Node contains compressed pointers to 4.5 bytes
struct RBNode
{
private:
    static constexpr UInt64 MAX_NODE_SIZE = 0xfffffffff; // 2 ^ 36, about 64 GB
    UInt8 data[24];
    // 4.5B (parent) | 4.5B (left) | 4.5B (right) | 4.5B (previous) | 4.5B (size) | 1b (color) | 1b (isFree) | 1b (isNextSet) | unused

public:
    enum class EColor : UInt8
    {
        Red,
        Black
    };

    RBNode *get_parent(Void* memory) const;
    RBNode *get_left(Void *memory) const;
    RBNode *get_right(Void *memory) const;
    // It calculates pointer based on size
    RBNode *get_next();
    RBNode *get_previous(Void *memory) const;
    UInt64 get_size() const;
    EColor get_color() const;
    Bool   is_free() const;
    Void   *get_memory();

    Void set_parent(RBNode *parent, Void *memory);
    Void set_left(RBNode *left, Void *memory);
    Void set_right(RBNode *right, Void *memory);
    // It sets isNextSet flag, false for nullptr, so remember to get_next before change size, also see get_next
    Void set_next(RBNode *next);
    Void set_previous(RBNode *previous, Void *memory);
    Void set_size(UInt64 size);
    Void set_color(EColor color);
    Void set_free(Bool isFree);

    Void reset();

    RBNode()
    {
        memset(data, 0, 24);
    }
};