#include "Core/Structures/rb_node.hpp"
#include "Memory/memory.hpp"
#include "Structures/rb_tree.hpp"

Int32 main()
{
    Void *mem = malloc(10_MiB);
    RBTree rbtree{ mem };
    RBNode *node7  = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 0 + 8 + 0 ); *node7  = {}; node7->set_size(7);
    RBNode *node3  = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 1 + 8 + 7 ); *node3  = {}; node3->set_size(3);
    RBNode *node18 = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 2 + 8 + 10); *node18 = {}; node18->set_size(18);
    RBNode *node10 = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 3 + 8 + 28); *node10 = {}; node10->set_size(10);
    RBNode *node22 = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 4 + 8 + 30); *node22 = {}; node22->set_size(22);
    RBNode *node8  = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 5 + 8 + 52); *node8  = {}; node8->set_size(8);
    RBNode *node11 = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 6 + 8 + 60); *node11 = {}; node11->set_size(11);
    RBNode *node26 = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 7 + 8 + 71); *node26 = {}; node26->set_size(26);
    RBNode *node2  = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 8 + 8 + 97); *node2  = {}; node2->set_size(2);
    RBNode *node6  = reinterpret_cast<RBNode *>(reinterpret_cast<UInt8*>(mem) + 32 * 9 + 8 + 99); *node6  = {}; node6->set_size(6);
    rbtree.insert(node7);
    rbtree.insert(node3);
    rbtree.insert(node18);
    rbtree.insert(node10);
    rbtree.insert(node22);
    rbtree.insert(node8);
    rbtree.insert(node11);
    rbtree.insert(node26);
    rbtree.insert(node2);
    rbtree.insert(node6);

    // Printing Red-Black Tree
    rbtree.printTree();

    // Deleting nodes from Red-Black Tree
    std::cout << "After deleting 18:" << std::endl;
    rbtree.remove(node18);
    rbtree.printTree();

    std::cout << "After deleting 11:" << std::endl;
    rbtree.remove(node11);
    rbtree.printTree();
    std::cout << "After deleting 3:" << std::endl;
    rbtree.remove(node3);
    rbtree.printTree();
    free(mem);
    return 0;
}
