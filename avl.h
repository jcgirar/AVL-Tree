/**
 * @file avl.h
 * @brief Intrusive, zero-allocation AVL balanced binary search tree library.
 *
 * This library implements self-balancing AVL trees where tree nodes
 * (`avl_node_t`) are embedded directly inside user-defined data structures.
 * It performs no dynamic memory allocation (`malloc`/`free`) and relies on
 * explicit fixed-size stacks (`avl_stack_t`) for non-recursive traversal,
 * insertion, removal, and iteration.
 */

#ifndef AVL_H_INCLUDED
#define AVL_H_INCLUDED 1

#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup macros Utility Macros
 * @{
 */

/**
 * @brief Retrieves a pointer to the parent structure containing an `avl_node_t`.
 *
 * @param ptr    Pointer to the embedded `avl_node_t` member.
 * @param type   Type of the container structure.
 * @param member Name of the `avl_node_t` member within the container structure.
 * @return Pointer to the containing structure of type `type *`.
 */
#define avl_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * @brief Retrieves a pointer to the parent structure using a dynamic byte offset.
 *
 * Useful when iterating generically over trees indexed by different members
 * of the same structure using `offsetof()`.
 *
 * @param ptr    Pointer to the embedded `avl_node_t` member.
 * @param type   Type of the container structure.
 * @param offset Byte offset of the `avl_node_t` member inside `type`.
 * @return Pointer to the containing structure of type `type *`.
 */
#define avl_container_in(ptr, type, offset) \
    ((type *)((char *)(ptr) - (offset)))

/**
 * @brief Overflow-safe three-way comparison helper macro.
 *
 * @param a First scalar value.
 * @param b Second scalar value.
 * @return `-1` if `a < b`, `1` if `a > b`, or `0` if `a == b`.
 */
#define avl_compare(a, b) (((a) < (b)) ? -1 : ((a) > (b)))

/**
 * @brief Static initializer for an `avl_tree_t` instance.
 *
 * @param func Comparison callback of type `avl_compare_t *`.
 */
#define avl_tree_init(func) { .compare = (func), .root = NULL }

/** @} */

/**
 * @defgroup types Core Data Structures
 * @{
 */

/**
 * @brief Intrusive AVL tree node.
 *
 * Embed this structure inside your custom data structure for each independent
 * index you want to maintain.
 */
typedef struct avl_node avl_node_t;
struct avl_node {
    avl_node_t *left;   /**< Pointer to the left child node. */
    avl_node_t *right;  /**< Pointer to the right child node. */
    int balance;        /**< Balance factor of the node (`height(right) - height(left)` or vice versa). */
};

/**
 * @brief Maximum traversal stack depth.
 *
 * Automatically scales to `64` on 64-bit targets (`UINTPTR_MAX > 0xffffffffU`)
 * and `32` on 32-bit targets to cover worst-case AVL tree heights without
 * recursion. Can be overridden at compile time via `-DAVL_STACK_SIZE=<N>`.
 */
#ifndef AVL_STACK_SIZE
    #if UINTPTR_MAX > 0xffffffffU
        #define AVL_STACK_SIZE 64
    #else
        #define AVL_STACK_SIZE 32
    #endif
#endif

/** @brief Pushes a node pointer reference onto an `avl_stack_t`. */
#define STACK_PUSH(stack, value) ((stack)->nodeptr[(stack)->top++] = (value))

/** @brief Pops a node pointer reference from an `avl_stack_t`. */
#define STACK_POP(stack) ((stack)->nodeptr[--(stack)->top])

/**
 * @brief Explicit traversal stack used to avoid recursion.
 *
 * Stores the path of node pointer references from the root down to a target
 * node, allowing `avl_tree_search()` to pass the exact path to
 * `avl_tree_insert()` or `avl_tree_remove()`.
 */
typedef struct avl_stack avl_stack_t;
struct avl_stack {
    avl_node_t **nodeptr[AVL_STACK_SIZE]; /**< Array of node link pointers along the path. */
    unsigned int top;                     /**< Current stack top index (number of stored items). */
};

/**
 * @brief Node comparison callback signature.
 *
 * @param a First node to compare.
 * @param b Second node to compare.
 * @return Negative value if `a < b`, positive if `a > b`, or `0` if equal.
 */
typedef int avl_compare_t(avl_node_t *a, avl_node_t *b);

/**
 * @brief AVL tree root descriptor.
 */
typedef struct avl_tree avl_tree_t;
struct avl_tree {
    avl_compare_t *compare; /**< Key comparison callback used to order nodes. */
    avl_node_t *root;       /**< Pointer to the root node of the tree, or `NULL` if empty. */
};

/** @} */

/**
 * @defgroup iterator Tree Iterator
 * @{
 */

/** @brief Iterate in descending order (right-to-left). */
#define AVL_ITERATOR_LEFT  0

/** @brief Iterate in ascending order (left-to-right). */
#define AVL_ITERATOR_RIGHT 1

/**
 * @brief Non-recursive in-order tree iterator.
 */
typedef struct {
    int direction;      /**< Traversal direction: `AVL_ITERATOR_RIGHT` (ascending) or `AVL_ITERATOR_LEFT` (descending). */
    avl_tree_t *index;  /**< Target AVL tree being traversed. */
    avl_stack_t stack;  /**< Internal traversal stack holding ancestor links. */
} avl_iterator_t;

/** @} */

/**
 * @defgroup api Public API Functions
 * @{
 */

/**
 * @brief Computes the height of the subtree rooted at `node`.
 *
 * @param node Pointer to an `avl_node_t` (may be `NULL`).
 * @return Height of the subtree (`0` if `node` is `NULL`).
 */
int avl_node_get_height(avl_node_t *node);

/**
 * @brief Computes the current balance factor of `node`.
 *
 * @param node Pointer to an `avl_node_t`.
 * @return Balance factor of the node.
 */
int avl_node_get_balance(avl_node_t *node);

/**
 * @brief Searches for a matching node in the tree.
 *
 * Records the search path inside `stack` so that a subsequent
 * call to `avl_tree_insert()` or `avl_tree_remove()` can reuse the traversal
 * without searching from the root again.
 *
 * @param tree  Pointer to the target `avl_tree_t`.
 * @param key   Dummy or template `avl_node_t` containing the key to search for.
 * @param stack Pointer to an `avl_stack_t` to store the path.
 * @return Pointer to the matching `avl_node_t` if found, or `NULL` otherwise.
 */
avl_node_t *avl_tree_search(avl_tree_t *tree, avl_node_t *key, avl_stack_t *stack);

/**
  * @brief Performs a fast, read-only lookup for a matching node in the tree.
  * 
  * Unlike `avl_tree_search()`, this function does not require or populate an
  * `avl_stack_t` and avoids double-pointer indirections, making it ideal for
  * pure queries where no subsequent insertion or removal will be performed.
  * 
  * @param tree Pointer to the target `avl_tree_t`.
  * @param key  Dummy or template `avl_node_t` containing the key to search for.
  * @return Pointer to the matching `avl_node_t` if found, or `NULL` otherwise.
  */
avl_node_t *avl_tree_find(avl_tree_t *tree, avl_node_t *key); 

/**
 * @brief Inserts a new node into the tree and rebalances it.
 *
 * If `stack` was previously populated by `avl_tree_search()`, insertion
 * proceeds directly at the recorded leaf position. Otherwise, pass `NULL`
 * to search from the root automatically.
 *
 * @param tree  Pointer to the target `avl_tree_t`.
 * @param node  Pointer to the `avl_node_t` to insert.
 * @param stack Optional pre-populated path from `avl_tree_search()`, or `NULL`.
 * @return `0` on success, or non-zero if a duplicate node exists or an error occurs.
 */
 int avl_tree_insert(avl_tree_t *tree, avl_node_t *node, avl_stack_t *stack);

/**
 * @brief Removes an existing node from the tree and rebalances it.
 *
 * @param tree  Pointer to the target `avl_tree_t`.
 * @param node  Pointer to the `avl_node_t` to remove (or key node to locate).
 * @param stack Optional pre-populated path from `avl_tree_search()`, or `NULL`.
 * @return `0` if the node was removed, or non-zero if not found.
 */
int avl_tree_remove(avl_tree_t *tree, avl_node_t *node, avl_stack_t *stack);

/**
 * @brief Initializes an `avl_iterator_t` for a given tree and direction.
 *
 * @param iter      Pointer to the iterator to initialize.
 * @param tree      Pointer to the `avl_tree_t` to traverse.
 * @param direction Traversal direction (`AVL_ITERATOR_RIGHT` or `AVL_ITERATOR_LEFT`).
 */
void avl_iterator_init(avl_iterator_t *iter, avl_tree_t *tree, int direction);

/**
 * @brief Checks whether the iterator still has elements left to visit.
 *
 * @param iter Pointer to an active `avl_iterator_t`.
 * @return Non-zero if more nodes are available, `0` when traversal is finished.
 */
int avl_iterator_have_data(avl_iterator_t *iter);

/**
 * @brief Positions the iterator at the first node according to its direction.
 *
 * @param iter Pointer to an `avl_iterator_t`.
 */
void avl_iterator_move_first(avl_iterator_t *iter);

/**
 * @brief Positions the iterator at the last node according to its direction.
 *
 * @param iter Pointer to an `avl_iterator_t`.
 */
void avl_iterator_move_last(avl_iterator_t *iter);

/**
 * @brief Positions the iterator at a specific node (or lower/upper bound) in the tree.
 *
 * @param iter Pointer to an `avl_iterator_t`.
 * @param node Target `avl_node_t` key to seek to.
 */
void avl_iterator_move(avl_iterator_t *iter, avl_node_t *node);

/**
 * @brief Returns the current node and advances the iterator to the next position.
 *
 * @param iter Pointer to an active `avl_iterator_t`.
 * @return Pointer to the current `avl_node_t`, or `NULL` if no data remains.
 */
avl_node_t *avl_iterator_get(avl_iterator_t *iter);

/** @} */

#endif /* AVL_H_INCLUDED */
