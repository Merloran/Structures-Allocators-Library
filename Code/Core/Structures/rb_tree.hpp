#pragma once

struct RBNode;

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

    Void insert(RBNode *node, Bool shouldCoalesce = true);

    Void remove(RBNode *node);

    RBNode *split_node(RBNode *node, UInt64 requestedBytes) const;

    Void coalesce(RBNode *node);

    RBNode *find(UInt64 size) const;

    Void print_tree();

    Void clear();

private:
    Bool contains(const RBNode *node) const;

    Void rotate_left(RBNode *node);

    Void rotate_right(RBNode *node);

    Void transplant(const RBNode *u, RBNode *v);

    RBNode *get_min(RBNode *node) const;

    Void fix_insert(const RBNode *node);

    Void fix_remove(RBNode *node);

    Void print_helper(const RBNode *node, std::string indent, Bool last);
};
