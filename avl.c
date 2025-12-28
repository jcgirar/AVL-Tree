#include <stddef.h>
#include <stdio.h>

#include "avl.h"
#include "avl_alt.h"

const int balances[5][5][2] = {
	{{1,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,1},{1,1},{0,1},{0,2},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{0,-2},{0,-2},{0,-1},{-1,-1},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{-1,0}}
};

void print_balances() {
	printf("\nint balances[5][5][2] = {\n");
	int a;
	for (a=0; a<5; a++) {
		printf("\t{");
		int b;
		for (b=0; b<5; b++) {
			printf("{%d,%d}", balances[a][b][0], balances[a][b][1]);
			if (b!=4) printf(",");
		}
		if (a==4)
			printf("}\n");
		else
			printf("},\n");
	}
	printf("};\n");
}

int get_height(avl_node_t *node) {
	unsigned int height = 0;
	if (node) {
		unsigned int rightheight = get_height(node->right);
		unsigned int leftheight = get_height(node->left);
		height = (rightheight > leftheight ? rightheight : leftheight);
	} 
	return height+1;
}

int get_balance(avl_node_t *node) {
	if (node) return get_height(node->right)-get_height(node->left);
	return 0;
}

avl_node_t **avl_getstack(avl_index_t *index, avl_stack_t *stack, avl_node_t *node) {
	stack->top = 0;				
	/* search the tree stacking the intermediate nodes */
	avl_node_t **inode = &index->root;
	while (*inode) {
		STACK_PUSH(stack, inode);
		int dif = index->compare(*inode, node);
		if (dif>0)
			inode = &(*inode)->left;
		else if (dif<0)
			inode = &(*inode)->right;
		else 
			/* found */
			return NULL;
	}
	/* not found so return the node where can insert new node*/
	return inode;
}

int	avl_init(avl_index_t *index, int (*compare)(avl_node_t *, avl_node_t *)) {
	if (index) {
		index->compare = compare;
		index->root = NULL;
		if (compare) index->getstack = avl_getstack;
		else index->getstack = avl_int_getstack;
		return 0;
	}
	return 1;
}

/* RIGHT ROTATION diagram
 * 
 *  before rotation:
 *  
 *         c
 *        / 
 *       b
 *      / \
 *     a   x
 * 
 *  after rotation:
 * 
 *        b
 *       / \
 *      a   c
 *         /
 *        x
 * */

static inline avl_node_t *rotate_right(avl_node_t *a) {
	/* get indexes to update balances */
	int ab = a->balance + 2;
	int bb = a->left->balance + 2;
	/*make rotation */
	avl_node_t *b = a->left; 
	a->left = b->right;
	b->right = a;
	/* update balances */
	a->balance = balances[ab][bb][0];
	b->balance = balances[ab][bb][1];
	return b;
}

/* LEFT ROTATION diagram
 * 
 *  before rotation:
 *  
 *     a    
 *      \
 *       b
 *      / \
 *     x   c
 * 
 *  after rotation:
 * 
 *        b
 *       / \
 *      a   c
 *       \  
 *        x
 * */

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

static inline void do_balance(avl_node_t **nodeptr) {
	avl_node_t *node = *nodeptr;
	avl_node_t *newnode = node;
	if (node->balance == -2) {
		avl_node_t *left = node->left;
		if (left->balance == -1) {
			/* case 1 */
			newnode = rotate_right(node);
		} else {
			/* case 2 */
			node->left = rotate_left(left);
			newnode = rotate_right(node);
		}
	} else if (node->balance == 2) {
		avl_node_t *right = node->right;
		if (right->balance == 1) {
			/* case 3*/
			newnode = rotate_left(node);
		} else {
			/* case 4 */
			node->right = rotate_right(right);
			newnode = rotate_left(node);
		}

	} else return;
	*nodeptr = newnode;
}

avl_node_t *avl_search(avl_index_t *index, avl_node_t *node) {
	avl_node_t *inode = index->root;
	while (inode) {
		int dif = index->compare(inode, node);
		if (dif>0)
			inode = inode->left;
		else if (dif<0)
			inode = inode->right;
		else {
			return inode;
		}
	}
	return NULL;
}


int avl_insert(avl_index_t *index, avl_node_t *node) {
	avl_stack_t stack;
	avl_node_t **inode = index->getstack(index, &stack, node);
	
    if (inode) {
		/* init node struct */
		node->balance = 0;
		node->left = NULL;
		node->right = NULL;
		/* insert the node in the last inode searched */
		*inode = node;
		/* after node inserted start balance */
		while (stack.top) {
			avl_node_t **parent = STACK_POP(&stack);		
			if (&(*parent)->left == inode)
				(*parent)->balance--;
			else
				(*parent)->balance++;	
			do_balance(parent);
			if ((*parent)->balance == 0) break;
			inode = parent;
		}
		/* return NOERROR*/ 
		return 0;
	}
	/* return ERROR*/
	return 1; 
}

static inline void trasverse_left(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->left;
	} 
}

static inline void trasverse_right(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->right;
	}
}

int avl_remove(avl_index_t *index, avl_node_t *node) {
	avl_stack_t stack;
	avl_node_t **inode = index->getstack(index, &stack, node);
	if (!inode) {
		inode = STACK_POP(&stack);
		if ((*inode)->right) {
			unsigned int tmpindex =++stack.top; 
			trasverse_left(&stack, &(*inode)->right);
			avl_node_t **substitute = STACK_POP(&stack);
			stack.nodeptr[tmpindex] = substitute;
			(*substitute)->left =(*inode)->left;
			(*substitute)->balance =(*inode)->balance;
			if (*substitute == (*inode)->right)
				*inode = *substitute;
			else {
				avl_node_t *tmp = (*substitute)->right;
				(*substitute)->right = (*inode)->right;
				*inode = *substitute;
				*substitute = tmp;
			}
			inode = substitute;
		} else {
			*inode = (*inode)->left;
		}
		/* after node deleted start balance */
		while (stack.top) {
			avl_node_t **parent = STACK_POP(&stack);
			if (&(*parent)->left == inode)
				(*parent)->balance++;
			else if (&(*parent)->right == inode)
				(*parent)->balance--;	
			do_balance(parent);
			if ((*parent)->balance != 0) break;
			inode = parent;
		}
		return 0;
	}
	return 1;
}

/**
 *   This function should be called ONLY after you have previously copied the
 * avl_node_t structure from orig to dest and also copied the key or keys used by the
 * index compare function in the container structure.
 * This means that avl_replace DO NOT reindex the tree, it just change the ponters
 * inside the tree. If you want change the keys just call avl_remove with orig and 
 * after that call avl_insert with dest but do not use avl_replace.
 *   Then yoy may ask what is the point of include this function at all. Remember that   
 * 
 */
int avl_replace(avl_index_t *index, avl_node_t *orig, avl_node_t *dest) {
	if (index && orig && dest) {
		avl_stack_t stack;
		avl_node_t **inode = index->getstack(index, &stack, orig);
		if (!inode) {
			inode = STACK_POP(&stack);
			*inode = dest;
			if (stack.top) {
				inode = STACK_POP(&stack);
				if ((*inode)->left == orig)
					(*inode)->left = dest;
				else
					(*inode)->right = dest;
			}	
			return 0;
		}
	}
	return 1;
}

/* iterator functions */

void avl_iterator_init(avl_iterator_t *iterator, avl_index_t *index, int direction) {	
	iterator->index = index;
	iterator ->direction = direction;
	iterator->stack.top = 0;
}

inline int avl_iterator_have_data(avl_iterator_t *iterator) {
	return iterator->stack.top;
}

void avl_iterator_move_first(avl_iterator_t *iterator) {
	avl_stack_t *stack = &iterator->stack;
	stack->top = 0;
	avl_node_t **root = &(iterator->index->root);
	trasverse_left(stack, root);
	if (!iterator->direction && stack->top) {
		stack->nodeptr[0] = stack->nodeptr[stack->top-1];
		stack->top = 1;
	}
}

void avl_iterator_move_last(avl_iterator_t *iterator) {
	avl_stack_t *stack = &iterator->stack;
	stack->top = 0;
	avl_node_t **root = &(iterator->index->root);
	trasverse_right(stack, root);
	if (iterator->direction && stack->top) {
		stack->nodeptr[0] = stack->nodeptr[stack->top-1];
		stack->top = 1;
	}
}

void avl_iterator_move(avl_iterator_t *iterator, avl_node_t *node) {
	avl_stack_t *stack = &iterator->stack;
	stack->top = 0;			
	avl_index_t *index = iterator->index;
	avl_node_t **inode = &index->root;
	while (*inode) {
		int dif = index->compare(*inode, node);
		if (dif>0) {
			if (iterator->direction) STACK_PUSH(stack,inode);; 
			inode = &(*inode)->left;
		} else if (dif<0) {
			if (!iterator->direction) STACK_PUSH(stack,inode);;
			inode = &(*inode)->right;
		} else {
			/* found */
			STACK_PUSH(stack,inode); 		
			return;
		}
	}
	stack->top = 0;
}

avl_node_t *avl_iterator_get(avl_iterator_t *iterator) {
	avl_stack_t *stack = &iterator->stack;
	if (stack->top) { 
		avl_node_t **current = stack->nodeptr[--stack->top];
		if (iterator->direction)
			trasverse_left(stack, &(*current)->right);
		else
			trasverse_right(stack, &(*current)->left);
		return *current;
	}
	return NULL;
}
