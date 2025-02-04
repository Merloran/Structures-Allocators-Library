#include "rb_tree.hpp"

Void RBTree::insert(RBNode* node)
{
    if (!node)
    {
        SPDLOG_WARN("Given node is nullptr!");
        return;
    }
    node->set_color(RBNode::EColor::Red);
    RBNode *parent = nullptr;
    RBNode *current = root;
    while (current != nullptr)
    {
        parent = current;
        if (node->get_size() < current->get_size())
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
    } else if (node->get_size() < parent->get_size())
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

RBNode* RBTree::split_node(RBNode* node, UInt64 requestedBytes)
{
    if (node->get_size() - requestedBytes <= 32)
    {
        return nullptr;
    }
    RBNode *splitNode = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8 *>(node->get_memory()) + requestedBytes);
    splitNode->set_size(node->get_size() - requestedBytes - sizeof(RBNode));
    node->set_size(requestedBytes);

    splitNode->set_free(true);
    splitNode->set_previous(node, memory);
    splitNode->set_next(node->get_next(memory), memory);
    node->set_next(splitNode, memory);
    return splitNode;
}

RBNode* RBTree::coalesce(RBNode* node)
{
    RBNode *current = node;

    for (RBNode *previous = current->get_previous(memory);
         previous && previous->is_free();
         previous = current->get_previous(memory))
    {
        current = previous;
    }

    remove(current);

    for (RBNode *next = current->get_next(memory);
         next && next->is_free();
         next = current->get_next(memory))
    {
        remove(next);

        current->set_next(next->get_next(memory), memory);
        current->set_size(current->get_size() + next->get_size() + sizeof(RBNode));
    }

    return current;
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

Bool RBTree::contains(const RBNode* node) const
{
    RBNode *current = root;
    const UInt64 size = current->get_size();
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
    while (node != root && 
           node->get_color() == RBNode::EColor::Red && 
           node->get_parent(memory)->get_color() == RBNode::EColor::Red) 
    {
        RBNode* parent = node->get_parent(memory);
        RBNode* grandparent = parent->get_parent(memory);
        if (parent == grandparent->get_left(memory))
        {
            RBNode *uncle = grandparent->get_right(memory);
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_right(memory))
                {
                    rotate_left(parent);
                    parent = grandparent;
                }
                rotate_right(grandparent);
                //TODO: there is something weird here
                const RBNode::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                node = parent;
            }
        } else {
            RBNode *uncle = grandparent->get_left(memory);
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_left(memory))
                {
                    rotate_right(parent);
                    parent = grandparent;
                }
                rotate_left(grandparent);
                //TODO: there is something weird here
                const RBNode::EColor tempColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(tempColor);
                node = parent;
            }
        }
    }
    root->set_color(RBNode::EColor::Black);

    coalesce(node);
}

Void RBTree::fix_remove(RBNode* node)
{
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
                sibling->set_color(parent->get_color());;
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