#pragma once

struct RBNode;
// TODO: Add support for RBNode 
class RBTree
{
private:
    Byte *memory;
    RBNode *root;

public:
    RBTree() noexcept
        : memory(nullptr)
        , root(nullptr)
    {}

    RBTree(Byte* allocatedMemory) noexcept
        : memory(allocatedMemory)
        , root(nullptr)
    {}

    Void insert(RBNode *node, Bool shouldCoalesce = true) noexcept;

    Void remove(RBNode *node) noexcept;

    RBNode *split_node(RBNode *node, USize requestedBytes, USize alignment) noexcept;

    Void coalesce(RBNode *node) noexcept;

    RBNode *find(USize size) const noexcept;

    Void print_tree() noexcept;

    Void clear() noexcept;

private:
    RBNode *align_node(RBNode *node, USize alignment) const noexcept;


    Void rotate_left(RBNode *node) noexcept;

    Void rotate_right(RBNode *node) noexcept;

    Void transplant(const RBNode *u, RBNode *v) noexcept;

    RBNode *get_min(RBNode *node) const noexcept;

    Void fix_insert(RBNode *node) noexcept;

    Void fix_remove(RBNode *node, RBNode *parent) noexcept;

    Void print_helper(const RBNode *node, std::string indent, Bool last) noexcept;

    // Mostly used for debug, because size can be duplicated and Its looking for node with specific address It needs to dfs tree sometimes
    Bool contains(const RBNode *node) const noexcept;
    // Debug stuff
    [[nodiscard]]
    Bool validate_tree() const noexcept;

    Bool validate_node(const RBNode *node, const RBNode *parent, Int32 &blackHeight,
                       const RBNode *minNode, const RBNode *maxNode) const noexcept;
    Int32 calculate_black_height(const RBNode *node) const noexcept;

};
