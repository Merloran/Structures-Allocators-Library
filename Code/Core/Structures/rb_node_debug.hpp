#pragma once
// This class is debug only use
struct RBNodeDebug
{
public:
    enum class EColor : UInt8
    {
        Red,
        Black
    };

private:
    RBNodeDebug *parent;
    RBNodeDebug *left;
    RBNodeDebug *right;
    RBNodeDebug *next;
    RBNodeDebug *previous;
    UInt64 size;
    EColor color;
    Bool isFree;

public:
    RBNodeDebug *get_parent(Void *memory) const { return parent; }
    RBNodeDebug *get_left(Void *memory) const { return left; }
    RBNodeDebug *get_right(Void *memory) const { return right; }
    RBNodeDebug *get_next(Void *memory) const { return next; }
    RBNodeDebug *get_previous(Void *memory) const { return previous; }
    UInt64 get_size() const { return size; }
    EColor get_color() const { return color; }
    Bool   is_free() const { return isFree; }
    Void *get_memory() { return reinterpret_cast<UInt8 *>(this) + sizeof(RBNodeDebug); }

    Void set_parent(RBNodeDebug *parent, Void *memory) { this->parent = parent; }
    Void set_left(RBNodeDebug *left, Void *memory) { this->left = left; }
    Void set_right(RBNodeDebug *right, Void *memory) { this->right = right; }
    Void set_next(RBNodeDebug *next, Void *memory) { this->next = next; }
    Void set_previous(RBNodeDebug *previous, Void *memory) { this->previous = previous; }
    Void set_size(UInt64 size) { this->size = size; }
    Void set_color(EColor color) { this->color = color; }
    Void set_free(Bool isFree) { this->isFree = isFree; }

    Void reset()
    {
        parent = left = right = nullptr;
        color = EColor::Red;
        isFree = true;
    }

    RBNodeDebug()
        : parent(nullptr)
        , left(nullptr)
        , right(nullptr)
        , next(nullptr)
        , previous(nullptr)
        , size(0)
        , color(EColor::Black)
        , isFree(true)
    {}
};