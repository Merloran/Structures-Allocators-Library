#include "rb_tree.hpp"

#include "rb_node.hpp"
#include "Serrate/Memory/memory_utils.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

Void RBTree::insert(RBNode* node, const Bool shouldCoalesce) noexcept
{
    if (!node)
    {
        SPDLOG_WARN("Given node is nullptr!");
        return;
    }
    node->reset();
    RBNode *parent = nullptr;
    const USize nodeSize = node->get_size();
    for (RBNode *current = root; current != nullptr;)
    {
        parent = current;
        if (nodeSize < current->get_size())
        {
            current = current->get_left();
        } else {
            current = current->get_right();
        }
    }
    node->set_parent(parent);
    if (parent == nullptr)
    {
        root = node;
    }
    else if (nodeSize < parent->get_size())
    {
        parent->set_left(node);
    } else {
        parent->set_right(node);
    }

    fix_insert(node);
    if (shouldCoalesce)
    {
        coalesce(node);
    }
}

Void RBTree::remove(RBNode* node) noexcept
{
    node->set_free(false);

    assert(contains(node));

    RBNode *x;
    RBNode *xParent;
    RBNode *y = node;
    RBNode::EColor yOriginalColor = y->get_color();
    if (!node->get_left())
    {
        x = node->get_right();
        xParent = node->get_parent();
        transplant(node, x);
    }
    else if (!node->get_right())
    {
        x = node->get_left();
        xParent = node->get_parent();
        transplant(node, x);
    } else {
        y = get_min(node->get_right());
        yOriginalColor = y->get_color();
        x = y->get_right();
        if (y->get_parent() == node)
        {
            xParent = y;
        } else {
            xParent = y->get_parent();
            transplant(y, x);
            RBNode *right = node->get_right();
            y->set_right(right);
            right->set_parent(y);
        }
        transplant(node, y);
        RBNode *left = node->get_left();
        y->set_left(left);
        left->set_parent(y);
        y->set_color(node->get_color());
    }

    if (yOriginalColor == RBNode::EColor::Black) 
    {
        fix_remove(x, xParent);
    }
}

RBNode* RBTree::split_node(RBNode* node, const USize requestedBytes, const USize alignment) noexcept
{
    node = align_node(node, alignment);

    if (node->get_size() - requestedBytes <= sizeof(RBNode))
    {
        return node;
    }

    RBNode *splitNode = Memory::start_object<RBNode>(node->get_memory() + requestedBytes);
    RBNode *next = node->get_next();
    splitNode->set_size(node->get_size() - (requestedBytes + sizeof(RBNode)));
    node->set_size(requestedBytes);
    splitNode->set_free(true);
    splitNode->set_previous(node);
    splitNode->set_next(next);
    if (next)
    {
        next->set_previous(splitNode);
    }
    node->set_next(splitNode);

    insert(splitNode, false);
    return node;
}

Void RBTree::coalesce(RBNode* node) noexcept
{
    RBNode *current = node;
    RBNode *previous = current->get_previous();
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
        const USize exactNodeSize = current->get_size() + sizeof(RBNode);
        previous->set_size(previous->get_size() + exactNodeSize);
        previous->set_next(next);
        current = previous;
        if (next)
        {
            next->set_previous(current);
        }
    }

    if (isNextFree)
    {
        remove(next);
        const USize exactNodeSize = next->get_size() + sizeof(RBNode);
        current->set_size(current->get_size() + exactNodeSize);
        next = next->get_next();
        current->set_next(next);
        if (next)
        {
            next->set_previous(current);
        }
    }

    insert(current, false);
}

RBNode* RBTree::find(const USize size) const noexcept
{
    RBNode *current = root;
    RBNode *bestFit = nullptr;

    while (current)
    {
        if (current->get_size() >= size)
        {
            bestFit = current;
            current = current->get_left();
        } else {
            current = current->get_right();
        }
    }

    assert(bestFit && "Failed to find enough size!");

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
    // print_helper(root, "", true);
}

Void RBTree::clear() noexcept
{
    root = nullptr;
    memory = nullptr;
}

RBNode* RBTree::align_node(RBNode* node, const USize alignment) const noexcept
{
    const USize padding = alignment - (USize(node->get_memory()) & (alignment - 1));

    if (padding == alignment) 
    {
        return node;
    }

    if (RBNode *previous = node->get_previous()) 
    {
        previous->set_size(previous->get_size() + padding);
    }
    node->set_size(node->get_size() - padding);

    Byte *newNode = byte_cast(node) + padding;
    memmove(newNode, node, sizeof(RBNode));

    return Memory::start_object<RBNode, false>(newNode);
}

Void RBTree::rotate_left(RBNode* node) noexcept
{
    RBNode *child = node->get_right();

    RBNode *right = child->get_left();
    node->set_right(right);
    if (right)
    {
        right->set_parent(node);
    }

    RBNode *parent = node->get_parent();
    child->set_parent(parent);
    if (!parent)
    {
        root = child;
    }
    else if (node == parent->get_left())
    {
        parent->set_left(child);
    } else {
        parent->set_right(child);
    }
    child->set_left(node);
    node->set_parent(child);
}

Void RBTree::rotate_right(RBNode* node) noexcept
{
    RBNode *child = node->get_left();
    RBNode *left = child->get_right();
    node->set_left(left);

    if (left)
    {
        left->set_parent(node);
    }

    RBNode *parent = node->get_parent();
    child->set_parent(parent);
    if (!parent)
    {
        root = child;
    }
    else if (node == parent->get_left())
    {
        parent->set_left(child);
    } else {
        parent->set_right(child);
    }
    child->set_right(node);
    node->set_parent(child);
}

Void RBTree::transplant(const RBNode* u, RBNode* v) noexcept
{
    RBNode *parent = u->get_parent();
    if (!parent)
    {
        root = v;
    }
    else if (u == parent->get_left())
    {
        parent->set_left(v);
    } else {
        parent->set_right(v);
    }

    if (v)
    {
        v->set_parent(parent);
    }
}

RBNode* RBTree::get_min(RBNode* node) const noexcept
{
    RBNode *current = node;
    RBNode *left = current->get_left();
    while (left)
    {
        current = left;
        left = current->get_left();
    }
    return current;
}

Void RBTree::fix_insert(RBNode* node) noexcept
{
    while (node != root &&
           node->get_color() == RBNode::EColor::Red &&
           node->get_parent()->get_color() == RBNode::EColor::Red)
    {
        RBNode* parent = node->get_parent();
        RBNode* grandparent = parent->get_parent();
        if (parent == grandparent->get_left())
        {
            RBNode *uncle = grandparent->get_right();
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_right())
                {
                    rotate_left(parent);
                    node = parent;
                    parent = node->get_parent();
                }
                rotate_right(grandparent);
                const RBNode::EColor parentColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(parentColor);
                node = parent;
            }
        } else {
            RBNode *uncle = grandparent->get_left();
            if (uncle && uncle->get_color() == RBNode::EColor::Red) 
            {
                grandparent->set_color(RBNode::EColor::Red);
                parent->set_color(RBNode::EColor::Black);
                uncle->set_color(RBNode::EColor::Black);
                node = grandparent;
            } else {
                if (node == parent->get_left())
                {
                    rotate_right(parent);
                    node = parent;
                    parent = node->get_parent();
                }
                rotate_left(grandparent);
                const RBNode::EColor parentColor = parent->get_color();
                parent->set_color(grandparent->get_color());
                grandparent->set_color(parentColor);
                node = parent;
            }
        }
    }
    root->set_color(RBNode::EColor::Black);
}

Void RBTree::fix_remove(RBNode* node, RBNode *parent) noexcept
{
    while (node != root && (!node || node->get_color() == RBNode::EColor::Black))
    {
        if (!parent)
        {
            break;
        }

        if (node == parent->get_left())
        {
            RBNode* sibling = parent->get_right();
            RBNode *left = nullptr;
            RBNode *right = nullptr;

            // Case 1: Sibling is red
            if (sibling)
            {
                if (sibling->get_color() == RBNode::EColor::Red)
                {
                    sibling->set_color(RBNode::EColor::Black);
                    parent->set_color(RBNode::EColor::Red);
                    rotate_left(parent);
                    sibling = parent->get_right();
                }
                left = sibling->get_left();
                right = sibling->get_right();
            }
            
            // Case 2: Sibling is black with two black children
            if (!sibling || 
               ((!left   || left->get_color() == RBNode::EColor::Black) && 
                (!right  || right->get_color() == RBNode::EColor::Black)))
            {
                if (sibling)
                {
                    sibling->set_color(RBNode::EColor::Red);
                }
                node = parent;
                parent = node->get_parent();
            }
            else if (sibling)
            {
                // Case 3: Sibling is black, left child is red, right child is black
                if (!right || right->get_color() == RBNode::EColor::Black)
                {
                    if (left)
                    {
                        left->set_color(RBNode::EColor::Black);
                    }
                    sibling->set_color(RBNode::EColor::Red);
                    rotate_right(sibling);
                    sibling = parent->get_right();
                    right = sibling->get_right();
                }

                // Case 4: Sibling is black, right child is red
                sibling->set_color(parent->get_color());
                parent->set_color(RBNode::EColor::Black);
                if (right)
                {
                    right->set_color(RBNode::EColor::Black);
                }
                rotate_left(parent);
                node = root;
            }
        } else {
            RBNode* sibling = parent->get_left();
            RBNode *left = nullptr;
            RBNode *right = nullptr;

            // Case 1: Sibling is red
            if (sibling)
            {
                if (sibling->get_color() == RBNode::EColor::Red)
                {
                    sibling->set_color(RBNode::EColor::Black);
                    parent->set_color(RBNode::EColor::Red);
                    rotate_right(parent);
                    sibling = parent->get_left();
                }
                left = sibling->get_left();
                right = sibling->get_right();
            }
            
            // Case 2: Sibling is black with two black children
            if (!sibling || 
               ((!left   || left->get_color() == RBNode::EColor::Black) &&
                (!right  || right->get_color() == RBNode::EColor::Black)))
            {
                if (sibling)
                {
                    sibling->set_color(RBNode::EColor::Red);
                }
                node = parent;
                parent = node->get_parent();
            }
            else if (sibling)
            {
                // Case 3: Sibling is black, right child is red, left child is black
                if (!left || left->get_color() == RBNode::EColor::Black)
                {
                    if (right)
                    {
                        right->set_color(RBNode::EColor::Black);
                    }
                    sibling->set_color(RBNode::EColor::Red);
                    rotate_left(sibling);
                    sibling = parent->get_left();
                    left = sibling->get_left();
                }
                
                // Case 4: Sibling is black, left child is red
                sibling->set_color(parent->get_color());
                parent->set_color(RBNode::EColor::Black);
                if (left)
                {
                    left->set_color(RBNode::EColor::Black);
                }
                rotate_right(parent);
                node = root;
            }
        }
    }
    
    if (node)
    {
        node->set_color(RBNode::EColor::Black);
    }
}


Bool RBTree::contains(const RBNode* node) const noexcept
{
    if (!node)
    {
        return false;
    }

    RBNode *current = root;
    const USize size = node->get_size();

    while (current)
    {
        if (current->get_size() >= size)
        {
            if (current == node)
            {
                return true;
            }
            current = current->get_left();
        } else {
            current = current->get_right();
        }
    }

    std::function<Bool(const RBNode *)> dfs = [&](const RBNode *subtreeRoot) -> Bool
    {
        if (!subtreeRoot)
        {
            return false;
        }

        if (subtreeRoot == node) 
        {
            return true;
        }

        return dfs(subtreeRoot->get_left()) ||
               dfs(subtreeRoot->get_right());
    };


    return dfs(root);
}

// Void RBTree::print_helper(const RBNode* node, std::string indent, const Bool last) noexcept
// {
//     if (node)
//     {
//         printf("%s", indent.c_str());
//         if (last)
//         {
//             printf("R----");
//             indent += "   ";
//         } else {
//             printf("L----");
//             indent += "|  ";
//         }
//         printf("%llu<%p>(%s)\n", node->get_size(), node, std::string(magic_enum::enum_name(node->get_color())).c_str());
//         print_helper(node->get_left(), indent, false);
//         print_helper(node->get_right(), indent, true);
//     }
// }

Bool RBTree::validate_tree() const noexcept
{
    if (!root) {
        SPDLOG_INFO("Tree is empty - valid");
        return true;
    }

    if (root->get_color() != RBNode::EColor::Black) {
        SPDLOG_ERROR("Root is not black!");
        return false;
    }

    if (root->get_parent() != nullptr) {
        SPDLOG_ERROR("Root has non-null parent!");
        return false;
    }

    Int32 blackHeight = -1;
    Bool result = validate_node(root, nullptr, blackHeight, nullptr, nullptr);

    if (result) {
        SPDLOG_INFO("Tree is valid! Black height: {}", blackHeight);
    } else {
        SPDLOG_ERROR("Tree validation failed!");
    }

    return result;
}

Bool RBTree::validate_node(const RBNode *node, const RBNode *parent, Int32 &blackHeight,
                           const RBNode *minNode, const RBNode *maxNode) const noexcept
{
    if (!node) {
        blackHeight = 0;
        return true;
    }

    if (node->get_parent() != parent) {
        SPDLOG_ERROR("Node {:p} has incorrect parent. Expected: {:p}, Got: {:p}",
                     (Void *)node, (Void *)parent, (Void*)node->get_parent());
        return false;
    }

    USize nodeSize = node->get_size();
    if (minNode && nodeSize < minNode->get_size()) {
        SPDLOG_ERROR("BST violation: Node {:p} size {} < min size {}",
                     (Void *)node, nodeSize, minNode->get_size());
        return false;
    }
    if (maxNode && nodeSize > maxNode->get_size()) {
        SPDLOG_ERROR("BST violation: Node {:p} size {} > max size {}",
                     (Void *)node, nodeSize, maxNode->get_size());
        return false;
    }

    if (node->get_color() == RBNode::EColor::Red) {
        if ((node->get_left() && node->get_left()->get_color() == RBNode::EColor::Red) ||
            (node->get_right() && node->get_right()->get_color() == RBNode::EColor::Red)) {
            SPDLOG_ERROR("Red node {:p} has red child!", (Void *)node);
            return false;
        }
    }

    if (node->get_left() && node->get_left()->get_parent() != node) {
        SPDLOG_ERROR("Left child of {:p} has incorrect parent pointer", (Void *)node);
        return false;
    }
    if (node->get_right() && node->get_right()->get_parent() != node) {
        SPDLOG_ERROR("Right child of {:p} has incorrect parent pointer", (Void *)node);
        return false;
    }

    Int32 leftBlackHeight = -1, rightBlackHeight = -1;

    const RBNode *leftMax = node;
    const RBNode *rightMin = node;

    if (!validate_node(node->get_left(), node, leftBlackHeight, minNode, leftMax)) {
        return false;
    }
    if (!validate_node(node->get_right(), node, rightBlackHeight, rightMin, maxNode)) {
        return false;
    }

    if (leftBlackHeight != rightBlackHeight) {
        SPDLOG_ERROR("Black height mismatch at node {:p}. Left: {}, Right: {}",
                     (Void *)node, leftBlackHeight, rightBlackHeight);
        return false;
    }

    blackHeight = leftBlackHeight;
    if (node->get_color() == RBNode::EColor::Black) {
        blackHeight++;
    }

    return true;
}

Int32 RBTree::calculate_black_height(const RBNode *node) const noexcept
{
    if (!node) {
        return 0;
    }

    Int32 height = calculate_black_height(node->get_left());
    if (node->get_color() == RBNode::EColor::Black) {
        height++;
    }

    return height;
}