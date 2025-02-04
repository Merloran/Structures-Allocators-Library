#pragma once

struct RBNode
{
private:
    static constexpr UInt64 MAX_NODE_SIZE = 0xfffffffff; // 2 ^ 36, about 64 GB
    UInt8 data[32];

public:
    enum class EColor : UInt8
    {
        Red,
        Black
    };

    RBNode *get_parent(Void* memory) const;
    RBNode *get_left(Void *memory) const;
    RBNode *get_right(Void *memory) const;
    RBNode *get_next(Void *memory) const;
    RBNode *get_previous(Void *memory) const;
    UInt64 get_size() const;
    EColor get_color() const;
    Bool   is_free() const;
    Void   *get_memory();

    Void set_parent(RBNode *parent, Void *memory);
    Void set_left(RBNode *left, Void *memory);
    Void set_right(RBNode *right, Void *memory);
    Void set_next(RBNode *next, Void *memory);
    Void set_previous(RBNode *previous, Void *memory);
    Void set_size(UInt64 size);
    Void set_color(EColor color);
    Void set_free(Bool isFree);

    RBNode()
    {
        memset(data, 0, 32);
    }
};