#pragma once
#include "rb_node.hpp"

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

    RBNode *coalesce(RBNode *node);

    RBNode *find(const UInt64 size) const;

private:
    Bool contains(const RBNode *node) const;

    Void rotate_left(RBNode *node);

    Void rotate_right(RBNode *node);

    Void transplant(const RBNode *u, RBNode *v);

    RBNode *get_min(RBNode *node) const;

    Void fix_insert(RBNode *node);

    Void fix_remove(RBNode *node);
};