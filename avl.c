/**
 * @file avl.c
 * @brief Non-recursive, intrusive AVL tree implementation in ISO C.
 *
 * This library does not perform dynamic memory allocation; the caller owns
 * and manages the memory of all container structures. Tree traversal,
 * rebalancing, and iteration are performed iteratively using explicit
 * pointer-to-pointer stacks (`avl_node_t **`).
 */
#include <stddef.h>

#include "avl.h"

/**
 * @brief Precomputed post-rotation balance factor lookup table.
 *
 * Indexed as `balances[a_balance + 2][b_balance + 2][node_index]`, where:
 * - `a_balance` is the pre-rotation balance factor of the pivot root `a` (`-2..2`).
 * - `b_balance` is the pre-rotation balance factor of the promoted child `b` (`-2..2`).
 * - `node_index = 0` yields the new balance factor for `a`, and `1` for `b`.
 */
const int balances[5][5][2] = {
	{{1,0},{0,0},{-1,1},{0,0},{0,0}},
	{{1,1},{1,1},{0,1},{0,2},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{0,-2},{0,-2},{0,-1},{-1,-1},{0,0}},
	{{0,0},{0,0},{1,-1},{0,0},{-1,0}}
};

/**
 * @brief Recursively computes the height of a subtree.
 *
 * @note Intended for debugging and invariant verification only; not used by
 *       core tree operations.
 *
 * @param node Root of the subtree (may be `NULL`).
 * @return Height of the subtree (`0` if `node` is `NULL`).
 */
int avl_node_get_height(avl_node_t *node) {
	unsigned int height = 0;
	if (node) {
		unsigned int rightheight = avl_node_get_height(node->right);
		unsigned int leftheight = avl_node_get_height(node->left);
		height = 1 + (rightheight > leftheight ? rightheight : leftheight);
	} 
	return height;
}

/**
 * @brief Recursively computes the balance factor (`height(right) - height(left)`).
 *
 * @note Intended for debugging and invariant verification only.
 *
 * @param node Node to inspect (may be `NULL`).
 * @return Balance factor of `node`, or `0` if `node` is `NULL`.
 */
int avl_node_get_balance(avl_node_t *node) {
	if (node) 
		return avl_node_get_height(node->right) - avl_node_get_height(node->left);
	return 0;
}

/**
 * @brief Performs a single right rotation around node `a`.
 *
 * @verbatim
 *   Before:          After:
 *       a              b
 *      /              / \
 *     b      ===>    c   a
 *    / \                /
 *   c   x              x
 * @endverbatim
 *
 * @param a Unbalanced subtree root whose left child `b` is promoted.
 * @return Pointer to the new subtree root (`b`).
 */
static inline avl_node_t *rotate_right(avl_node_t *a) {
	/* get indexes to update balances */
	int ab = a->balance + 2;
	int bb = a->left->balance + 2;
	/* make rotation */
	avl_node_t *b = a->left; 
	a->left = b->right;
	b->right = a;
	/* update balances */
	a->balance = balances[ab][bb][0];
	b->balance = balances[ab][bb][1];
	return b;
}

/**
 * @brief Performs a single left rotation around node `a`.
 *
 * @verbatim
 *   Before:          After:
 *     a                b
 *      \              / \
 *       b    ===>    a   c
 *      / \            \
 *     x   c            x
 * @endverbatim
 *
 * @param a Unbalanced subtree root whose right child `b` is promoted.
 * @return Pointer to the new subtree root (`b`).
 */
static inline avl_node_t *rotate_left(avl_node_t *a) {
	int ab = a->balance + 2;
	int bb = a->right->balance + 2;
	/* make rotation */
	avl_node_t *b = a->right;
	a->right = b->left;
	b->left = a;
	/* update balances */
	a->balance = balances[ab][bb][0];
	b->balance = balances[ab][bb][1];
	return b;
}

/**
 * @brief Rebalances `node` if its balance factor has reached `-2` or `+2`.
 *
 * Automatically performs double rotations (Left-Right or Right-Left) when the
 * heavy child leans in the opposite direction.
 *
 * @param node Subtree root to rebalance.
 * @return Pointer to the balanced subtree root.
 */
static inline avl_node_t *balance_node(avl_node_t *node) {
	switch (node->balance) {
		case -2:
			if (node->left->balance == 1) node->left = rotate_left(node->left);
			return rotate_right(node);
		case 2:
			if (node->right->balance == -1) node->right = rotate_right(node->right);
			return rotate_left(node);
		default:
			break;
	}
	return node;
}

/**
 * @copydoc avl_tree_search
 */
avl_node_t *avl_tree_search(avl_tree_t *tree, avl_node_t *key, avl_stack_t *stack) {
	if (tree && tree->compare && key && stack) {
		avl_node_t **inode = &tree->root;
		stack->top = 1
		stack->nodeptr[0] = inode;
		/* search the tree stacking the intermediate nodes */
		while (*inode) {
			int dif = tree->compare(*inode, key);
			if (dif > 0) 
				inode = &(*inode)->left;
			else if (dif < 0) 
				inode = &(*inode)->right;
			else   
				return *inode;
			STACK_PUSH(stack, inode);
		}
	}
	return NULL;
}

/**
 * @copydoc avl_tree_find
 */
avl_node_t *avl_tree_find(avl_tree_t *tree, avl_node_t *key) {
	if (tree && tree->compare && key) {
		avl_node_t *current = tree->root;
		while (current) {
			int dif = tree->compare(current, key);
			if (dif > 0) 
				current = current->left;
			else if (dif < 0) 
				current = current->right;
			else   
				return current;
		}
	}
	return NULL;
}


/**
 * @copydoc avl_tree_insert
 */
int avl_tree_insert(avl_tree_t *tree, avl_node_t *node, avl_stack_t *stack) {
	if (tree && node) {
		avl_stack_t ts;
		if (!stack) {
			stack = &ts;
			stack->top = 0; /* init stack to avoid corrupted memory in local variable */
			avl_tree_search(tree, node, stack); /* ignore function result because we will use stack */
		}
		if (!stack->top) return 1; /* if stack is empty return */
	    avl_node_t **inode = STACK_POP(stack);
		/* not possible to insert on an already occupied inode */
		if (*inode) return 1; 
		/* init node struct */
		node->balance = 0;
		node->left = NULL;
		node->right = NULL;
		/* insert the node in the last empty inode stacked */
		*inode = node;
		/* start balance */
		while (stack->top) {
			avl_node_t **parentptr = STACK_POP(stack);
			avl_node_t *parent = *parentptr;		
			if (&(parent->left) == inode)
				parent->balance--;
			else
				parent->balance++;	
			*parentptr = balance_node(parent);
			/* if current node is balanced then no further balances necessary */
			if ((*parentptr)->balance == 0) break;
			inode = parentptr;
		}
		/* return NOERROR */
		return 0;
	}
	/* return ERROR */ 
	return 1; 
}

/**
 * @brief Descends down the leftmost spine from `inode`, pushing all links onto `stack`.
 */
static inline void traverse_left(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->left;
	} 
}

/**
 * @brief Descends down the rightmost spine from `inode`, pushing all links onto `stack`.
 */
static inline void traverse_right(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->right;
	}
}

/**
 * @copydoc avl_tree_remove
 *
 * @details
 * Removal is handled in two phases: standard BST node unlinking (4 cases)
 * followed by bottom-up stack rebalancing:
 *
 * - **Case (a) — No right child (`p->right == NULL`):**
 *   Replaces `p` with `p->left` (or `NULL` if `p` is a leaf).
 *
 * - **Case (b) — Only right child (`p->left == NULL`):**
 *   Replaces `p` with `p->right`.
 *
 * - **Case (c) — Two children, immediate right child is in-order successor (`p2 == p->right`):**
 *   `p2` takes `p`'s left child and balance factor, and directly replaces `p`.
 *
 * - **Case (d) — Two children, deeper in-order successor (`p2 != p->right`):**
 *   Finds the leftmost node `p2` in `p->right`, patches the stack entry that
 *   pointed to `p->right` so it points to `&p2->right`, promotes `p2` into `p`'s
 *   slot, and attaches `p2->right` to `p2`'s former parent (`*sus`).
 *
 * @verbatim
 *   Case (d) example (deleting *b, successor is *i):
 *
 *           __a__                           __a__
 *          /     \                         /     \
 *        *b       c                       i       c
 *        / \     / \        ===>         / \     / \
 *       d   e   f   g                   d   e   f   g
 *      /   / \       \                 /   / \       \
 *     h  *i   j       k               h   l   j       k
 *          \
 *           l
 * @endverbatim
 */

int avl_tree_remove(avl_tree_t *tree, avl_node_t *node, avl_stack_t *stack) {
	if (tree && node) {
		avl_stack_t ts;
		if (!stack) {
			stack = &ts;
			stack->top = 0; /* init stack to avoid corrupted memory in local variable */
			avl_tree_search(tree, node, stack); /* ignore function result because we will use stack */
		}
		if (!stack->top) return 1; /* if stack is empty return */
	    avl_node_t **inode = STACK_POP(stack);
		/* not possible to delete an empty inode */
		if (!(*inode)) return 1;
		/* after initial checks init the remove process */
		avl_node_t *p = *inode;		
		if (p->right) { 
			if (p->left) {
				STACK_PUSH(stack,inode);
				unsigned int t = stack->top;
				traverse_left(stack, &p->right);
				avl_node_t **sus = STACK_POP(stack);
				avl_node_t *p2 = *sus;
				if (stack->top > t) stack->nodeptr[t] = &p2->right;				
				p2->left = p->left;
				p2->balance = p->balance;
				if (p2 == p->right) {
					/* case c*/
					*inode = p2;
					inode = &(p2->right);	
				} else {
					/* case d */
					*sus = p2->right;
					p2->right = p->right;
					*inode = p2;
					inode = sus;
				}
			} else {
				/* case b */
				*inode = p->right;
			}
		} else {
			/* case a */
			*inode = p->left;
		}
		/* after node deleted start balance using the resulting stack */	
		while (stack->top) {
			avl_node_t **parentptr = STACK_POP(stack);
			avl_node_t *parent = *parentptr;
			if (&(parent->left) == inode) {
				parent->balance++;
			} else if (&(parent->right) == inode) {
				parent->balance--;	
			}
			*parentptr = balance_node(parent);
			/* if balance of node is diferent to 0 that means that the previous
			 * balance was 0 so it is not necesary to adjust balance the previous
			 * nodes (see WIKIPEDIA avl trees delete operation) */
			if ((*parentptr)->balance != 0) break;
			inode = parentptr; 
		}
		return 0;
	}
	return 1;
}

/**
 * @copydoc avl_iterator_init
 */
void avl_iterator_init(avl_iterator_t *iterator, avl_tree_t *tree, int direction) {
	if (iterator && tree) {	
		iterator->index = tree;
		iterator->direction = direction;
		iterator->stack.top = 0;
	}
}

/**
 * @copydoc avl_iterator_have_data
 */
inline int avl_iterator_have_data(avl_iterator_t *iterator) {
	if (iterator) return iterator->stack.top;
	return 0;
}

/**
 * @copydoc avl_iterator_move_first
 *
 * @details
 * Seeks the leftmost (minimum key) node in the tree. When traversing in
 * descending order (`AVL_ITERATOR_LEFT`), intermediate left-spine ancestors
 * are discarded so that subsequent calls to `avl_iterator_get()` terminate
 * immediately after yielding the minimum node.
 */
void avl_iterator_move_first(avl_iterator_t *iterator) {
	if (iterator && iterator->index) {
		avl_stack_t *stack = &iterator->stack;
		stack->top = 0;
		avl_node_t **root = &(iterator->index->root);
		traverse_left(stack, root);
		if (!iterator->direction && stack->top) {
			stack->nodeptr[0] = stack->nodeptr[stack->top-1];
			stack->top = 1;
		}
	}
}

/**
 * @copydoc avl_iterator_move_last
 *
 * @details
 * Seeks the rightmost (maximum key) node in the tree. When traversing in
 * ascending order (`AVL_ITERATOR_RIGHT`), intermediate right-spine ancestors
 * are discarded so that subsequent calls to `avl_iterator_get()` terminate
 * immediately after yielding the maximum node.
 */
void avl_iterator_move_last(avl_iterator_t *iterator) {
	if (iterator && iterator->index) {
		avl_stack_t *stack = &iterator->stack;
		stack->top = 0;
		avl_node_t **root = &(iterator->index->root);
		traverse_right(stack, root);
		if (iterator->direction && stack->top) {
			stack->nodeptr[0] = stack->nodeptr[stack->top-1];
			stack->top = 1;
		}
	}
}

/**
 * @copydoc avl_iterator_move
 *
 * @details
 * Records ancestor turns relevant to the configured `direction` while descending
 * the tree. On an exact match, the matching node is pushed onto the stack and
 * yielded first. If `key` is not present, the stack naturally retains the
 * closest in-order successor (`AVL_ITERATOR_RIGHT`) or predecessor
 * (`AVL_ITERATOR_LEFT`), or remains empty if `key` lies past the end of the tree.
 */
void avl_iterator_move(avl_iterator_t *iterator, avl_node_t *key) {
	if (iterator && iterator->index && iterator->index->compare && key) {
		avl_stack_t *stack = &iterator->stack;
		stack->top = 0;			
		avl_tree_t *tree = iterator->index;
		avl_node_t **inode = &tree->root;
		while (*inode) {
			int dif = tree->compare(*inode, key);
			if (dif > 0) {
				if (iterator->direction) STACK_PUSH(stack, inode); 
				inode = &(*inode)->left;
			} else if (dif < 0) {
				if (!iterator->direction) STACK_PUSH(stack, inode);
				inode = &(*inode)->right;
			} else {
				/* found */
				STACK_PUSH(stack, inode); 		
				return;
			}
		}
	}
}

/**
 * @copydoc avl_iterator_get
 */
avl_node_t *avl_iterator_get(avl_iterator_t *iterator) {
	if (iterator) {
		avl_stack_t *stack = &iterator->stack;
		if (stack->top) { 
			avl_node_t **current = STACK_POP(stack);
			if (iterator->direction)
				traverse_left(stack, &(*current)->right);
			else
				traverse_right(stack, &(*current)->left);
			return *current;
		}
	}
	return NULL;
}
