#include "rb_tree.hpp"
#include "rb_node_packed.hpp"
#include <magic_enum/magic_enum.hpp>

Void RBTree::insert(RBNodePacked* node, const Bool shouldCoalesce) noexcept
{
    if (!node)
    {
        SPDLOG_WARN("Given node is nullptr!");
        return;
    }
    node->reset();
    RBNodePacked *parent = nullptr;
    RBNodePacked *current = root;
    const USize nodeSize = node->get_size();
    while (current != nullptr)
    {
        parent = current;
        if (nodeSize < current->get_size())
        {
            current = current->get_left(memory);
        } else {
            current = current->get_right(memory);
        }
    }
    node->set_parent(parent, memory);
    if (parent == nullptr)
    {
        root = node;
    } else if (nodeSize < parent->get_size())
    {
        parent->set_left(node, memory);
    } else {
        parent->set_right(node, memory);
    }

    fix_insert(node);
    if (shouldCoalesce)
    {
        coalesce(node);
    }
}

Void RBTree::remove(RBNodePacked* node) noexcept
{
    node->set_free(false);

    if (!contains(node)) 
    {
        return;
    }

    RBNodePacked *x;
    RBNodePacked *y = node;
    RBNodePacked::EColor yOriginalColor = y->get_color();
    if (!node->get_left(memory))
    {
        x = node->get_right(memory);
        transplant(node, x);
    }
    else if (!node->get_right(memory))
    {
        x = node->get_left(memory);
        transplant(node, x);
    } else {
        y = get_min(node->get_right(memory));
        yOriginalColor = y->get_color();
        x = y->get_right(memory);
        if (y->get_parent(memory) == node)
        {
            if (x)
            {
                x->set_parent(y, memory);
            }
        } else {
            transplant(y, x);
            RBNodePacked *right = node->get_right(memory);
            y->set_right(right, memory);
            right->set_parent(y, memory);
        }
        transplant(node, y);
        RBNodePacked *left = node->get_left(memory);
        y->set_left(left, memory);
        left->set_parent(y, memory);
        y->set_color(node->get_color());
    }

    if (yOriginalColor == RBNodePacked::EColor::Black) 
    {
        fix_remove(x);
    }
}

RBNodePacked* RBTree::split_node(RBNodePacked* node, const USize requestedBytes, const USize alignment) noexcept
{
    node = align_node(node, alignment);

    if (node->get_size() - requestedBytes <= sizeof(RBNodePacked))
    {
        return node;
    }

    RBNodePacked *splitNode = Memory::start_object<RBNodePacked>(node->get_memory() + requestedBytes);
    RBNodePacked *next = node->get_next();
    splitNode->set_size(node->get_size() - (requestedBytes + sizeof(RBNodePacked)));
    node->set_size(requestedBytes);
    splitNode->set_free(true);
    splitNode->set_previous(node, memory);
    splitNode->set_next(next);
    if (next)
    {
        next->set_previous(splitNode, memory);
    }
    node->set_next(splitNode);

    insert(splitNode, false);
    return node;
}

Void RBTree::coalesce(RBNodePacked* node) noexcept
{
    RBNodePacked *current = node;
    RBNodePacked *previous = current->get_previous(memory);
    RBNodePacked *next = current->get_next();
    Bool isNextFree = false, isPreviousFree = false;

    if (previous)
    {
        isPreviousFree = previous->is_free();
    }

    if (next)
    {
        isNextFree = next->is_free();
    }

    if (!isPreviousFree && !isNextFree)
    {
        return;
    }

    remove(current);
    if (isPreviousFree)
    {
        remove(previous);
        const USize exactNodeSize = current->get_size() + sizeof(RBNodePacked);
        previous->set_size(previous->get_size() + exactNodeSize);
        previous->set_next(next);
        current = previous;
        if (next)
        {
            next->set_previous(current, memory);
        }
    }

    if (isNextFree)
    {
        remove(next);
        const USize exactNodeSize = next->get_size() + sizeof(RBNodePacked);
        current->set_size(current->get_size() + exactNodeSize);
        next = next->get_next();
        current->set_next(next);
        if (next)
        {
            next->set_previous(current, memory);
        }
    }

    insert(current, false);
}

RBNodePacked* RBTree::find(const USize size) const noexcept
{
    RBNodePacked *current = root;
    RBNodePacked *bestFit = nullptr;

    while (current)
    {
        if (current->get_size() >= size)
        {
            bestFit = current;
            current = current->get_left(memory);
        } else {
            current = current->get_right(memory);
        }
    }

    if (!bestFit)
    {
        SPDLOG_WARN("Failed to find enough size!");
    }

    return bestFit;
}

Void RBTree::print_tree() noexcept
{
    if (!root)
    {
        SPDLOG_INFO("Tree is empty.");
        return;
    }

    SPDLOG_INFO("Red-Black Tree:");
    print_helper(root, "", true);
}

Void RBTree::clear() noexcept
{
    root = nullptr;
    memory = nullptr;
}

RBNodePacked* RBTree::align_node(RBNodePacked* node, const USize alignment) const noexcept
{
    const USize padding = alignment - (USize(node->get_memory()) & (alignment - 1));

    if (padding == alignment) 
    {
        return node;
    }

    if (RBNodePacked *previous = node->get_previous(memory)) 
    {
        previous->set_size(previous->get_size() + padding);
    }
    node->set_size(node->get_size() - padding);

    Byte *newNode = byte_cast(node) + padding;
    memmove(newNode, node, sizeof(RBNodePacked));

    return Memory::start_object<RBNodePacked, false>(newNode);
}

Bool RBTree::contains(const RBNodePacked* node) const noexcept
{
    RBNodePacked *current = root;
    const USize size = node->get_size();
    while (current)
    {
        if (current->get_size() > size)
        {
            current = current->get_left(memory);
        } else {
            if (current == node)
            {
                return true;
            }
            current = current->get_right(memory);
        }
    }

    return false;
}

Void RBTree::rotate_left(RBNodePacked* node) noexcept
{
    RBNodePacked *child = node->get_right(memory);

    RBNodePacked *right = child->get_left(memory);
    node->set_right(right, memory);
    if (right)
    {
        right->set_parent(node, memory);
    }

    RBNodePacked *parent = node->get_parent(memory);
    child->set_parent(parent, memory);
    if (!parent)
    {
        root = child;
    }
    else if (node == parent->get_left(memory))
    {
        parent->set_left(child, memory);
    } else {
        parent->set_right(child, memory);
    }
    child->set_left(node, memory);
    node->set_parent(child, memory);
}

Void RBTree::rotate_right(RBNodePacked* node) noexcept
{
    RBNodePacked *child = node->get_left(memory);
    RBNodePacked *left = child->get_right(memory);
    node->set_left(left, memory);

    if (left)
    {
        left->set_parent(node, memory);
    }

    RBNodePacked *parent = node->get_parent(memory);
    child->set_parent(parent, memory);
    if (!parent)
    {
        root = child;
    }
    else if (node == parent->get_left(memory))
    {
        parent->set_left(child, memory);
    } else {
        parent->set_right(child, memory);
    }
    child->set_right(node, memory);
    node->set_parent(child, memory);
}

Void RBTree::transplant(const RBNodePacked* u, RBNodePacked* v) noexcept
{
    RBNodePacked *parent = u->get_parent(memory);
    if (!parent)
    {
        root = v;
    }
    else if (u == parent->get_left(memory))
    {
        parent->set_left(v, memory);
    } else {
        parent->set_right(v, memory);
    }

    if (v)
    {
        v->set_parent(parent, memory);
    }
}

RBNodePacked* RBTree::get_min(RBNodePacked* node) const noexcept
{
    RBNodePacked *current = node;
    RBNodePacked *left = current->get_left(memory);
    while (left)
    {
        current = left;
        left = current->get_left(memory);
    }
    return current;
}

Void RBTree::fix_insert(RBNodePacked* node) noexcept
{
    while (node != root &&
           node->get_color() == RBNodePacked::EColor::Red &&
           node->get_parent(memory)->get_color() == RBNodePacked::EColor::Red)
    {
        RBNodePacked* parent = node->get_parent(memory);
        RBNodePacked* grandparent = parent->get_parent(memory);
        if (parent == grandparent->get_left(memory))
        {
            RBNodePacked *uncle = grandparent->get_right(memory);
            if (uncle && uncle->get_color() == RBNodePacked::EColor::Red) 
            {
                grandparent->set_color(RBNodePacked::EColor::Red);
                parent->set_color(RBNodePacked::EColor::Black);
                uncle->set_color(RBNodePacked::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_right(memory))
                {
                    rotate_left(parent);
                    node = parent;
                    parent = node->get_parent(memory);
                }
                rotate_right(grandparent);
                const RBNodePacked::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                node = parent;
            }
        } else {
            RBNodePacked *uncle = grandparent->get_left(memory);
            if (uncle && uncle->get_color() == RBNodePacked::EColor::Red) 
            {
                grandparent->set_color(RBNodePacked::EColor::Red);
                parent->set_color(RBNodePacked::EColor::Black);
                uncle->set_color(RBNodePacked::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_left(memory))
                {
                    rotate_right(parent);
                    node = parent;
                    parent = node->get_parent(memory);
                }
                rotate_left(grandparent);
                const RBNodePacked::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                node = parent;
            }
        }
    }
    root->set_color(RBNodePacked::EColor::Black);
}

Void RBTree::fix_remove(RBNodePacked* node) noexcept
{
    if (node == nullptr)
    {
        return;
    }

    while (node != root && node->get_color() == RBNodePacked::EColor::Black) 
    {
        RBNodePacked *parent = node->get_parent(memory);
        if (node == parent->get_left(memory))
        {
            RBNodePacked *sibling = parent->get_right(memory);
            if (sibling->get_color() == RBNodePacked::EColor::Red) 
            {
                sibling->set_color(RBNodePacked::EColor::Black);
                parent->set_color(RBNodePacked::EColor::Red);
                rotate_left(parent);
                sibling = parent->get_right(memory);
            }

            parent = node->get_parent(memory);
            RBNodePacked *left = sibling->get_left(memory);
            RBNodePacked *right = sibling->get_right(memory);

            if ((!left || left->get_color() == RBNodePacked::EColor::Black) && 
                (!right || right->get_color() == RBNodePacked::EColor::Black)) 
            {
                sibling->set_color(RBNodePacked::EColor::Red);
                node = parent;
            } else {
                if (!right || right->get_color() == RBNodePacked::EColor::Black)
                {
                    if (left)
                    {
                        left->set_color(RBNodePacked::EColor::Black);
                    }
                    sibling->set_color(RBNodePacked::EColor::Red);
                    rotate_right(sibling);
                    parent = node->get_parent(memory);
                    sibling = parent->get_right(memory);
                }
                sibling->set_color(parent->get_color());
                parent->set_color(RBNodePacked::EColor::Black);
                right = sibling->get_right(memory);
                if (right)
                {
                    right->set_color(RBNodePacked::EColor::Black);
                }
                rotate_left(parent);
                node = root;
            }
        } else {
            RBNodePacked *sibling = parent->get_left(memory);
            if (sibling->get_color() == RBNodePacked::EColor::Red) 
            {
                sibling->set_color(RBNodePacked::EColor::Black);
                parent->set_color(RBNodePacked::EColor::Red);
                rotate_right(parent);
                sibling = parent->get_left(memory);
            }

            parent = node->get_parent(memory);
            RBNodePacked *left = sibling->get_left(memory);
            RBNodePacked *right = sibling->get_right(memory);

            if ((!left || left->get_color() == RBNodePacked::EColor::Black) && 
                (!right || right->get_color() == RBNodePacked::EColor::Black)) 
            {
                sibling->set_color(RBNodePacked::EColor::Red);
                node = parent;
            } else {
                if (!left || left->get_color() == RBNodePacked::EColor::Black)
                {
                    if (right)
                    {
                        right->set_color(RBNodePacked::EColor::Black);
                    }
                    sibling->set_color(RBNodePacked::EColor::Red);
                    rotate_left(sibling);
                    parent = node->get_parent(memory);
                    sibling = parent->get_left(memory);
                }
                sibling->set_color(parent->get_color());
                parent->set_color(RBNodePacked::EColor::Black);
                left = sibling->get_left(memory);
                if (left)
                {
                    left->set_color(RBNodePacked::EColor::Black);
                }
                rotate_right(parent);
                node = root;
            }
        }
    }
    node->set_color(RBNodePacked::EColor::Black);
}

Void RBTree::print_helper(const RBNodePacked* node, std::string indent, const Bool last) noexcept
{
    if (node)
    {
        printf("%s", indent.c_str());
        if (last)
        {
            printf("R----");
            indent += "   ";
        } else {
            printf("L----");
            indent += "|  ";
        }
        printf("%llu(%s)\n", node->get_size(), std::string(magic_enum::enum_name(node->get_color())).c_str());
        print_helper(node->get_left(memory), indent, false);
        print_helper(node->get_right(memory), indent, true);
    }
}