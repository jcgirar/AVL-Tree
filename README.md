# Intrusive AVL Tree Library in ISO C
![C Build & Test](https://github.com)

A lightweight, **non-recursive**, **intrusive**, and **zero-allocation** AVL self-balancing binary search tree library written in standard ISO C.

Unlike conventional implementations that wrap user payloads behind `void *` pointers and rely on `malloc`/`free`, this library embeds the tree node (`avl_node_t`) directly inside your own `struct` definitions (similar to Linux kernel intrusive data structures). This eliminates heap fragmentation, improves cache locality, and lets a single record belong to multiple sorted trees simultaneously without duplicating data.

## Key Features

* **Intrusive Design (`avl_container_of` / `avl_container_in`):** Nodes live inside your user structs, avoiding extra pointer indirections and separate node allocations.
* **Zero Dynamic Memory Allocation:** The library never calls `malloc()` or `free()`. You retain 100% control over where your data lives (stack, static arrays, arenas, or memory pools), making it ideal for embedded and real-time systems.
* **Multi-Index Support:** Embed multiple `avl_node_t` fields inside a single `struct` to keep records simultaneously indexed by different keys (e.g., ID, full name, and age).
* **Non-Recursive & Stack-Driven (`avl_stack_t`):** Traversal, insertion, rebalancing, and deletion use explicit fixed-depth pointer-to-pointer stacks (`AVL_STACK_SIZE = 32`, supporting trees with millions of nodes without recursion).
* **Reusable Search Path:** `avl_tree_search()` can record the traversal path into an `avl_stack_t` and pass it directly to `avl_tree_insert()`, avoiding a second tree traversal when checking for duplicate keys.
* **O(1) Table-Driven Rebalancing:** Post-rotation balance factors are resolved via a precomputed lookup table (`balances[5][5][2]`) instead of height recalculations.
* **Bidirectional Iterators (`avl_iterator_t`):** Step through trees in ascending (`AVL_ITERATOR_RIGHT`) or descending (`AVL_ITERATOR_LEFT`) order, or seek directly to a specific key.

## Project Structure

```text
├── avl.h        # Public API, macros, node/tree structs, and iterator definitions
├── avl.c        # Non-recursive AVL rotations, search, insert, remove, and iterators
└── example.c    # Multi-index usage example indexing records by ID, name, and age
```

## Building & Integration

There are no external dependencies beyond the C standard library (`<stddef.h>`). Simply copy `avl.h` and `avl.c` into your project and compile:

```bash
gcc -O2 -Wall -Wextra example.c avl.c -o avl_example
./avl_example
```

## Quick Start

### 1. Embed `avl_node_t` in your structure
Add one `avl_node_t` member per independent index you want to maintain:

```c
#include "avl.h"

struct person {
    unsigned int id;
    char name[16];
    unsigned int age;

    /* Intrusive tree nodes */
    avl_node_t id_node;
    avl_node_t age_node;
};
```

### 2. Write comparison callbacks
Use `avl_container_of()` to retrieve the parent structure from an `avl_node_t *` and `avl_compare()` for overflow-safe scalar comparisons:

```c
int id_compare(avl_node_t *a, avl_node_t *b) {
    struct person *pa = avl_container_of(a, struct person, id_node);
    struct person *pb = avl_container_of(b, struct person, id_node);
    return avl_compare(pa->id, pb->id);
}
```

### 3. Initialize trees, insert, and remove nodes
You can check for uniqueness with `avl_tree_search()` and reuse the recorded `avl_stack_t` on insertion, or pass `NULL` to insert/remove directly:

```c
avl_tree_t id_tree = avl_tree_init(id_compare);

int add_person(struct person *p) {
    avl_stack_t stack;
    if (avl_tree_search(&id_tree, &p->id_node, &stack) != NULL) {
        return 1; /* Duplicate key */
    }
    return avl_tree_insert(&id_tree, &p->id_node, &stack);
}

int remove_person(struct person *p) {
    return avl_tree_remove(&id_tree, &p->id_node, NULL);
}
```

### 4. Traverse with iterators
Use `AVL_ITERATOR_RIGHT` for ascending order or `AVL_ITERATOR_LEFT` for descending order:

```c
void print_tree(avl_tree_t *tree, size_t node_offset) {
    avl_iterator_t iter;
    avl_iterator_init(&iter, tree, AVL_ITERATOR_RIGHT);

    avl_iterator_move_first(&iter);
    while (avl_iterator_have_data(&iter)) {
        avl_node_t *node = avl_iterator_get(&iter);
        struct person *p = avl_container_in(node, struct person, node_offset);
        printf("ID: %u | Name: %s | Age: %u\n", p->id, p->name, p->age);
    }
}
```

## API Reference

### Macros & Initialization

| Symbol | Description |
| :--- | :--- |
| `avl_tree_init(func)` | Static initializer for an `avl_tree_t` with comparison callback `func`. |
| `avl_container_of(ptr, type, member)` | Casts an `avl_node_t *` back to its enclosing `type *` structure. |
| `avl_container_in(ptr, type, offset)` | Casts an `avl_node_t *` back to `type *` using a runtime `offsetof()` value. |
| `avl_compare(a, b)` | Overflow-safe three-way scalar comparison returning `-1`, `0`, or `1`. |

### Tree Operations

| Function | Description | Time Complexity |
| :--- | :--- | :--- |
| `avl_tree_search(tree, key, stack)` | Searches for `key` and optionally records the traversal path in `stack`. | $O(\log n)$ |
| `avl_tree_insert(tree, node, stack)` | Inserts `node` and rebalances the tree (reuses `stack` if provided). Returns `0` on success. | $O(\log n)$ |
| `avl_tree_remove(tree, node, stack)` | Removes `node` and rebalances the tree. Returns `0` on success, `1` if not found. | $O(\log n)$ |
| `avl_node_get_height(node)` | Recursively computes subtree height (debugging/verification helper). | $O(n)$ |
| `avl_node_get_balance(node)` | Recursively computes node balance factor (debugging/verification helper). | $O(n)$ |

### Iterator Operations

| Function | Description |
| :--- | :--- |
| `avl_iterator_init(iter, tree, dir)` | Initializes an iterator for `tree` (`AVL_ITERATOR_RIGHT` = ascending, `AVL_ITERATOR_LEFT` = descending). |
| `avl_iterator_move_first(iter)` | Moves the iterator to the minimum node in the tree. |
| `avl_iterator_move_last(iter)` | Moves the iterator to the maximum node in the tree. |
| `avl_iterator_move(iter, key)` | Seeks an exact match for `key` and prepares in-order traversal from that node. |
| `avl_iterator_have_data(iter)` | Returns non-zero while unvisited nodes remain in the iterator stack. |
| `avl_iterator_get(iter)` | Returns the current `avl_node_t *` and advances to the next node. |

## License

Distributed under the MIT License. See `LICENSE` for more information.
