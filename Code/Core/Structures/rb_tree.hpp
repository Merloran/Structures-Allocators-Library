#pragma once
#include <iostream>

#if defined(_DEBUG)
// #include "rb_node_debug.hpp"
// using RBNode = RBNodeDebug;
// #else
#include "rb_node.hpp"
#endif

class RBTree
{
private:
    Void *memory;
    RBNode *root;

public:
    RBTree()
        : memory(nullptr)
        , root(nullptr)
    {}

    RBTree(Void* allocatedMemory)
        : memory(allocatedMemory)
        , root(nullptr)
    {}

    Void insert(RBNode *node);

    Void remove(RBNode *node);

    RBNode *split_node(RBNode *node, UInt64 requestedBytes);

    Void coalesce(RBNode *node);

    RBNode *find(const UInt64 size) const;

    void printTree()
    {
        if (root == nullptr)
            std::cout << "Tree is empty." << std::endl;
        else {
            std::cout << "Red-Black Tree:" << std::endl;
            printHelper(root, "", true);
        }
    }

    // Utility function: Helper to print Red-Black Tree
    void printHelper(RBNode *root, std::string indent, bool last)
    {
        if (root != nullptr) {
            std::cout << indent;
            if (last) {
                std::cout << "R----";
                indent += "   ";
            } else {
                std::cout << "L----";
                indent += "|  ";
            }
            std::string sColor
                = (root->get_color() == RBNode::EColor::Red) ? "RED" : "BLACK";
            std::cout << root->get_size() << "(" << sColor << ")" << std::endl;
            printHelper(root->get_left(memory), indent, false);
            printHelper(root->get_right(memory), indent, true);
        }
    }

    Void clear();

private:
    Bool contains(const RBNode *node) const;

    Void rotate_left(RBNode *node);

    Void rotate_right(RBNode *node);

    Void transplant(const RBNode *u, RBNode *v);

    RBNode *get_min(RBNode *node) const;

    Void fix_insert(RBNode *node);

    Void fix_remove(RBNode *node);
};