#pragma once

struct RBNodePacked;
// TODO: Add support for RBNode 
class RBTree
{
private:
    Byte *memory;
    RBNodePacked *root;

public:
    RBTree() noexcept
        : memory(nullptr)
        , root(nullptr)
    {}

    RBTree(Byte* allocatedMemory) noexcept
        : memory(allocatedMemory)
        , root(nullptr)
    {}

    Void insert(RBNodePacked *node, Bool shouldCoalesce = true) noexcept;

    Void remove(RBNodePacked *node) noexcept;

    RBNodePacked *split_node(RBNodePacked *node, USize requestedBytes) const noexcept;

    Void coalesce(RBNodePacked *node) noexcept;

    RBNodePacked *find(USize size) const noexcept;

    Void print_tree() noexcept;

    Void clear() noexcept;

private:
    Bool contains(const RBNodePacked *node) const noexcept;

    Void rotate_left(RBNodePacked *node) noexcept;

    Void rotate_right(RBNodePacked *node) noexcept;

    Void transplant(const RBNodePacked *u, RBNodePacked *v) noexcept;

    RBNodePacked *get_min(RBNodePacked *node) const noexcept;

    Void fix_insert(const RBNodePacked *node) noexcept;

    Void fix_remove(RBNodePacked *node) noexcept;

    Void print_helper(const RBNodePacked *node, std::string indent, Bool last) noexcept;
};
