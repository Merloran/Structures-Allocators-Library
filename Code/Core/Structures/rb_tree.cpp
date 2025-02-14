#include "rb_tree.hpp"

#if 0 & defined(_DEBUG) // erase 0 & for better debug view, but results will be a bit different because of bigger node size
#include "rb_node_debug.hpp"
using RBNode = RBNodeDebug;
#else
#include "rb_node.hpp"
#endif

#include <magic_enum.hpp>

Void RBTree::insert(RBNode* node)
{
    if (!node)
    {
        SPDLOG_WARN("Given node is nullptr!");
        return;
    }
    node->reset();
    RBNode *parent = nullptr;
    RBNode *current = root;
    const UInt64 nodeSize = node->get_size();
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
}

Void RBTree::remove(RBNode* node)
{
    node->set_free(false);

    if (!contains(node)) 
    {
        return;
    }

    RBNode *x;
    RBNode *y = node;
    RBNode::EColor yOriginalColor = y->get_color();
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
            RBNode *right = node->get_right(memory);
            y->set_right(right, memory);
            right->set_parent(y, memory);
        }
        transplant(node, y);
        RBNode *left = node->get_left(memory);
        y->set_left(left, memory);
        left->set_parent(y, memory);
        y->set_color(node->get_color());
    }

    if (yOriginalColor == RBNode::EColor::Black) 
    {
        fix_remove(x);
    }
}

RBNode* RBTree::split_node(RBNode* node, UInt64 requestedBytes) const
{
    if (node->get_size() - requestedBytes <= sizeof(RBNode))
    {
        return nullptr;
    }
    RBNode *splitNode = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(node->get_memory()) + requestedBytes);
    splitNode->set_size(node->get_size() - (requestedBytes + sizeof(RBNode)));
    node->set_size(requestedBytes);
    splitNode->set_free(true);
    splitNode->set_previous(node, memory);
    splitNode->set_next(node->get_next());
    node->set_next(splitNode);
    return splitNode;
}

Void RBTree::coalesce(RBNode* node)
{
    RBNode *current = node;
    RBNode *previous = current->get_previous(memory);
    RBNode *next = current->get_next();
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
        const UInt64 exactNodeSize = current->get_size() + sizeof(RBNode);
        previous->set_size(previous->get_size() + exactNodeSize);
        previous->set_next(next);
        current = previous;
    }

    if (isNextFree)
    {
        remove(next);
        const UInt64 exactNodeSize = next->get_size() + sizeof(RBNode);
        current->set_size(current->get_size() + exactNodeSize);
        current->set_next(next->get_next());
    }

    insert(current);
}

RBNode* RBTree::find(const UInt64 size) const
{
    RBNode *current = root;
    RBNode *bestFit = nullptr;

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

Void RBTree::print()
{
    if (!root)
    {
        SPDLOG_INFO("Tree is empty.");
        return;
    }

    SPDLOG_INFO("Red-Black Tree:");
    spdlog::set_pattern("%v");
    print_helper(root, "", true);
    spdlog::set_pattern("%+");
}

Void RBTree::clear()
{
    root = nullptr;
    memory = nullptr;
}

Bool RBTree::contains(const RBNode* node) const
{
    RBNode *current = root;
    const UInt64 size = node->get_size();
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

Void RBTree::rotate_left(RBNode* node)
{
    RBNode *child = node->get_right(memory);

    RBNode *right = child->get_left(memory);
    node->set_right(right, memory);
    if (right)
    {
        right->set_parent(node, memory);
    }

    RBNode *parent = node->get_parent(memory);
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

Void RBTree::rotate_right(RBNode* node)
{
    RBNode *child = node->get_left(memory);
    RBNode *left = child->get_right(memory);
    node->set_left(left, memory);

    if (left)
    {
        left->set_parent(node, memory);
    }

    RBNode *parent = node->get_parent(memory);
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

Void RBTree::transplant(const RBNode* u, RBNode* v)
{
    RBNode *parent = u->get_parent(memory);
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

RBNode* RBTree::get_min(RBNode* node) const
{
    RBNode *current = node;
    RBNode *left = current->get_left(memory);
    while (left)
    {
        current = left;
        left = current->get_left(memory);
    }
    return current;
}

Void RBTree::fix_insert(RBNode* node)
{
    RBNode *current = node;
    while (current != root &&
           current->get_color() == RBNode::EColor::Red &&
           current->get_parent(memory)->get_color() == RBNode::EColor::Red)
    {
        RBNode* parent = current->get_parent(memory);
        RBNode* grandparent = parent->get_parent(memory);
        if (parent == grandparent->get_left(memory))
        {
            RBNode *uncle = grandparent->get_right(memory);
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                current = grandparent;
            } else {
                if (current == parent->get_right(memory))
                {
                    rotate_left(parent);
                    current = parent;
                    parent = current->get_parent(memory);
                }
                rotate_right(grandparent);
                const RBNode::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                current = parent;
            }
        } else {
            RBNode *uncle = grandparent->get_left(memory);
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                current = grandparent;
            } else {
                if (current == parent->get_left(memory))
                {
                    rotate_right(parent);
                    current = parent;
                    parent = current->get_parent(memory);
                }
                rotate_left(grandparent);
                const RBNode::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                current = parent;
            }
        }
    }
    root->set_color(RBNode::EColor::Black);

    coalesce(node);
}

Void RBTree::fix_remove(RBNode* node)
{
    if (node == nullptr)
    {
        return;
    }

    while (node != root && node->get_color() == RBNode::EColor::Black) 
    {
        RBNode *parent = node->get_parent(memory);
        if (node == parent->get_left(memory))
        {
            RBNode *sibling = parent->get_right(memory);
            if (sibling->get_color() == RBNode::EColor::Red) 
            {
                sibling->set_color(RBNode::EColor::Black);
                parent->set_color(RBNode::EColor::Red);
                rotate_left(parent);
                sibling = parent->get_right(memory);
            }

            parent = node->get_parent(memory);
            RBNode *left = sibling->get_left(memory);
            RBNode *right = sibling->get_right(memory);

            if ((!left || left->get_color() == RBNode::EColor::Black) && 
                (!right || right->get_color() == RBNode::EColor::Black)) 
            {
                sibling->set_color(RBNode::EColor::Red);
                node = parent;
            } else {
                if (!right || right->get_color() == RBNode::EColor::Black)
                {
                    if (left)
                    {
                        left->set_color(RBNode::EColor::Black);
                    }
                    sibling->set_color(RBNode::EColor::Red);
                    rotate_right(sibling);
                    parent = node->get_parent(memory);
                    sibling = parent->get_right(memory);
                }
                sibling->set_color(parent->get_color());
                parent->set_color(RBNode::EColor::Black);
                right = sibling->get_right(memory);
                if (right)
                {
                    right->set_color(RBNode::EColor::Black);
                }
                rotate_left(parent);
                node = root;
            }
        } else {
            RBNode *sibling = parent->get_left(memory);
            if (sibling->get_color() == RBNode::EColor::Red) 
            {
                sibling->set_color(RBNode::EColor::Black);
                parent->set_color(RBNode::EColor::Red);
                rotate_right(parent);
                sibling = parent->get_left(memory);
            }

            parent = node->get_parent(memory);
            RBNode *left = sibling->get_left(memory);
            RBNode *right = sibling->get_right(memory);

            if ((!left || left->get_color() == RBNode::EColor::Black) && 
                (!right || right->get_color() == RBNode::EColor::Black)) 
            {
                sibling->set_color(RBNode::EColor::Red);
                node = parent;
            } else {
                if (!left || left->get_color() == RBNode::EColor::Black)
                {
                    if (right)
                    {
                        right->set_color(RBNode::EColor::Black);
                    }
                    sibling->set_color(RBNode::EColor::Red);
                    rotate_left(sibling);
                    parent = node->get_parent(memory);
                    sibling = parent->get_left(memory);
                }
                sibling->set_color(parent->get_color());
                parent->set_color(RBNode::EColor::Black);
                left = sibling->get_left(memory);
                if (left)
                {
                    left->set_color(RBNode::EColor::Black);
                }
                rotate_right(parent);
                node = root;
            }
        }
    }
    node->set_color(RBNode::EColor::Black);
}

Void RBTree::print_helper(const RBNode* node, std::string indent, const Bool last)
{
    if (node)
    {
        SPDLOG_INFO(indent);
        if (last)
        {
            SPDLOG_INFO("R----");
            indent += "   ";
        } else {
            SPDLOG_INFO("L----");
            indent += "|  ";
        }
        SPDLOG_INFO("{}({})\n", node->get_size(), magic_enum::enum_name(node->get_color()));
        print_helper(node->get_left(memory), indent, false);
        print_helper(node->get_right(memory), indent, true);
    }
}