#include "rb_node.hpp"

RBNode* RBNode::get_parent() const noexcept
{
    return parent;
}

RBNode* RBNode::get_left() const noexcept
{
    return left;
}

RBNode* RBNode::get_right() const noexcept
{
    return right;
}

RBNode* RBNode::get_next() const noexcept
{
    if (isNextSet)
    {
        const USize exactNodeSize = sizeof(RBNode) + size;
        return reinterpret_cast<RBNode *>(byte_cast(const_cast<RBNode *>(this) + exactNodeSize));
    }
    return nullptr;
}

RBNode* RBNode::get_previous() const noexcept
{
    return previous;
}

USize RBNode::get_size() const noexcept
{
    return size;
}

RBNode::EColor RBNode::get_color() const noexcept
{
    return color;
}

Bool RBNode::is_free() const noexcept
{
    return isFree;
}

Byte* RBNode::get_memory() const noexcept
{
    return byte_cast(const_cast<RBNode *>(this)) + sizeof(RBNode);
}

Void RBNode::set_parent(RBNode* nodeParent) noexcept
{
    parent = nodeParent;
}

Void RBNode::set_left(RBNode* leftChild) noexcept
{
    left = leftChild;
}

Void RBNode::set_right(RBNode* rightChild) noexcept
{
    right = rightChild;
}

Void RBNode::set_next(const RBNode* nextNode) noexcept
{
    if (nextNode != nullptr)
    {
        isNextSet = true;
    } else {
        isNextSet = false;
    }
}

Void RBNode::set_previous(RBNode* previousNode) noexcept
{
    previous = previousNode;
}

Void RBNode::set_size(USize nodeSize) noexcept
{
    size = nodeSize;
}

Void RBNode::set_color(EColor nodeColor) noexcept
{
    color = nodeColor;
}

Void RBNode::set_free(Bool isNodeFree) noexcept
{
    isFree = isNodeFree;
}

Void RBNode::reset()
{
    parent = left = right = nullptr;
    color = EColor::Red;
    isFree = true;
    isNextSet = false;
}
