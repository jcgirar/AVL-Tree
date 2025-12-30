#include <stddef.h>
#include <stdio.h>

#include "avl.h"

const int balances[5][5][2] = {
	{{1,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,1},{1,1},{0,1},{0,2},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{0,-2},{0,-2},{0,-1},{-1,-1},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{-1,0}}
};
/*
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
*/

/* this two recursive functions are used to test if the balances are correct
 * but are not used by the other functions */
int avl_node_get_height(avl_node_t *node) {
	unsigned int height = 0;
	if (node) {
		unsigned int rightheight = avl_node_get_height(node->right);
		unsigned int leftheight = avl_node_get_height(node->left);
		height = 1 + (rightheight > leftheight ? rightheight : leftheight);
	} 
	return height;
}

int avl_node_get_balance(avl_node_t *node) {
	if (node) 
		return avl_node_get_height(node->right) - avl_node_get_height(node->left);
	return 0;
}

/* function used when no compare callback are used , it asumes the key is an int
 * and its placed just after the avl_node_t . Its an expeiment to test if without
 * callbacks the insert-delete proccess is faster */
static avl_node_t **_int_getstack(avl_tree_t *index, avl_stack_t *stack, avl_node_t *node) {
	stack->top = 0;				
	/* search the tree stacking the intermediate nodes */
	avl_node_t **inode = &index->root;
	while (*inode) {
		STACK_PUSH(stack, inode);
		unsigned int a = AVL_UINTKEY(*inode);
		unsigned int b = AVL_UINTKEY(node);
		if (a>b)
			inode = &(*inode)->left;
		else if (a<b)
			inode = &(*inode)->right;
		else 
			/* found */
			return NULL;
	}
	/* not found so return the node where can insert new node*/
	return inode;
}


avl_node_t **avl_getstack(avl_tree_t *index, avl_stack_t *stack, avl_node_t *node) {
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

int	avl_tree_init(avl_tree_t *index, int (*compare)(avl_node_t *, avl_node_t *)) {
	if (index) {
		index->compare = compare;
		index->root = NULL;
		if (compare) index->getstack = avl_getstack;
		else index->getstack = _int_getstack;
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

avl_node_t *avl_tree_search(avl_tree_t *index, avl_node_t *node) {
	avl_node_t *inode = index->root;
	while (inode) {
		int dif = index->compare(inode, node);
		if (dif>0)
			inode = inode->left;
		else if (dif<0)
			inode = inode->right;
		else break;
	}
	return inode;
}


int avl_tree_insert(avl_tree_t *index, avl_node_t *node) {
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
			avl_node_t **parentptr = STACK_POP(&stack);
			avl_node_t *parent = *parentptr;		
			if (&(parent->left) == inode)
				parent->balance--;
			else
				parent->balance++;	
			*parentptr = balance_node(parent);
			if ((*parentptr)->balance == 0) break;
			inode = parentptr;
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

/* this function is not working yet */
int avl_tree_remove(avl_tree_t *index, avl_node_t *node) {
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
			avl_node_t **parentptr = STACK_POP(&stack);
			avl_node_t *parent = *parentptr;
			if (&(parent->left) == inode)
				parent->balance++;
			else if (&(parent->right) == inode)
				parent->balance--;	
			*parentptr = balance_node(parent);
			if ((*parentptr)->balance != 0) break;
			inode = parentptr;
		}
		return 0;
	}
	return 1;
}

/**
 *   This function should be called ONLY after you have previously copied the
 * avl_node_t structure from orig to dest and also copied the key or keys used by the
 * index compare function in the container structure.
 * This means that avl_tree_replace DO NOT reindex the tree, it just change the ponters
 * inside the tree. If you want change the keys just call avl_tree_remove with orig and 
 * after that call avl_insert with dest but do not use avl_tree_replace.
 *   Then yoy may ask what is the point of include this function at all. Remember that   
 * 
 */
int avl_tree_replace(avl_tree_t *index, avl_node_t *orig, avl_node_t *dest) {
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

void avl_iterator_init(avl_iterator_t *iterator, avl_tree_t *index, int direction) {	
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
	avl_tree_t *index = iterator->index;
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
