#pragma once

struct RBNode
{
public:
    enum class EColor : UInt8
    {
        Red,
        Black
    };

private:
    RBNode *parent;
    RBNode *left;
    RBNode *right;
    RBNode *previous;
    USize size;
    EColor color;
    Bool isFree;
    Bool isNextSet;

public:
    RBNode() noexcept
        : parent(nullptr)
        , left(nullptr)
        , right(nullptr)
        , previous(nullptr)
        , size(0)
        , color(EColor::Black)
        , isFree(true)
        , isNextSet(false)
    {}

    [[nodiscard]]
    RBNode *get_parent() const noexcept;
    [[nodiscard]]
    RBNode *get_left() const noexcept;
    [[nodiscard]]
    RBNode *get_right() const noexcept;
    [[nodiscard]]
    RBNode *get_next() const noexcept;
    [[nodiscard]]
    RBNode *get_previous() const noexcept;
    [[nodiscard]]
    USize   get_size() const noexcept;
    [[nodiscard]]
    EColor  get_color() const noexcept;
    [[nodiscard]]
    Bool    is_free() const noexcept;
    [[nodiscard]]
    Byte   *get_memory() const noexcept;

    Void set_parent(RBNode *nodeParent) noexcept;
    Void set_left(RBNode *leftChild) noexcept;
    Void set_right(RBNode *rightChild) noexcept;
    Void set_next(const RBNode *nextNode) noexcept;
    Void set_previous(RBNode *previousNode) noexcept;
    Void set_size(USize nodeSize) noexcept;
    Void set_color(EColor nodeColor) noexcept;
    Void set_free(Bool isNodeFree) noexcept;

    Void reset();
};