#include <stddef.h>
#include <stdio.h>

#include "avl.h"

const int balances[5][5][2] = {
	{{1,0},{0,0},{-1,1},{0,0},{0,0}},
	{{1,1},{1,1},{0,1},{0,2},{0,0}},
	{{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{0,-2},{0,-2},{0,-1},{-1,-1},{0,0}},
	{{0,0},{0,0},{1,-1},{0,0},{-1,0}}
};

/* void print_balances() {
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
}*/


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

int	avl_tree_init(avl_tree_t *index, int (*compare)(avl_node_t *, avl_node_t *)) {
	if (index && compare) {
		index->compare = compare;
		index->root = NULL;
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

avl_node_t *avl_tree_search(avl_tree_t *index, avl_node_t *node, avl_stack_t *stack) {
	if (index && node) {
		avl_node_t **inode = &index->root;
		int dif;
		if (stack) 
			stack->top =0;
		/* search the tree stacking the intermediate nodes */
		do {
			if (stack) STACK_PUSH(stack, inode);
			if (*inode) {
				dif = index->compare(*inode, node);
				if (dif>0) 
					inode = &(*inode)->left;
				else if (dif<0) 
					inode = &(*inode)->right;
				else   
					return *inode;
			} else break;
		} while (1);
	}
	return NULL;
}


int avl_tree_insert(avl_tree_t *index, avl_node_t *node, avl_stack_t *stack) {
	avl_stack_t ts;
	if (!stack) {
		stack = &ts;
		/* if node found return because no insert is possible */
		if (avl_tree_search(index,node,stack)) return 1;
	}
	avl_node_t **inode = STACK_POP(stack);
	/* not possible to insert on an arready ocupied inode  */
	if (*inode) return 1;
	/* init node struct */
	node->balance = 0;
	node->left = NULL;
	node->right = NULL;
	/* insert the node in the last inode stacked */
	*inode = node;
	/*start balance */
	while (stack->top) {
		avl_node_t **parentptr = STACK_POP(stack);
		avl_node_t *parent = *parentptr;		
		if (&(parent->left) == inode)
			parent->balance--;
		else
			parent->balance++;	
		*parentptr = balance_node(parent);
		/* if current node is balanced then no further balances necesary */
		if ((*parentptr)->balance == 0) break;
		inode = parentptr;
	}
	/* return NOERROR*/ 
	return 0; 
}

static inline void traverse_left(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->left;
	} 
}

static inline void traverse_right(avl_stack_t *stack, avl_node_t **inode) {
	while (*inode) {
		STACK_PUSH(stack, inode);
		inode = &(*inode)->right;
	}
}


/* function avl_tree_remove is divided in two parts: one is remove the node from
 * the tree that its common with any other type of BST tree. And the second is balance
 * the tree after deletion. I writed both parts but in the same function but maybe
 * in the future i divide it in to functions (sugesions?)
 * 
 * this is a little bit tricky to understand but easy once you visualize it.
 * There are 4 cases:
 * 
 * a) the node have only a left child or no child at all. For example node d. Just subtitute the 
 * 		node by its left child. In case of node d is subtituted by node h. In case of nodes
 *		f,h,i,j and k are substituted by NULL so node its deleted.
 * 
 * b) the node have only right child. For example node g. This this is done the same way thar
 * 		case a, but instead of left node substitute deleted node by its right node.
 *
 * If node have both children then trasverse the tree nodes to find the next in order node.
 * and save to the stack the intermediate nodes. Also substitute in the stack the address
 * of the right node of deleted by the address of right node of the susbtitute node. copy 
 * to the substitute node the left node and the balance of deleted node.
 * 
 * c) If the next node is the right leaf of the node to be deleted. 
 * 
 * d) This is the example node with * in the diagram. ( NOTE: I have to complete this doc)  
 * 
 *           __a__                           __a__
 *          /     \                         /     \
 *        *b       c                       i       c
 *        / \     / \                     / \     / \
 *       d   e   f   g                   d   e   f   g
 *      /   / \       \                 /   / \       \
 *     h  *i   j       k               h   l   j       k
 *          \                               
 *           l                               
 */
int avl_tree_remove(avl_tree_t *index, avl_node_t *node, avl_stack_t *stack) {
	avl_stack_t ts;
	if (!stack) stack = &ts;
	if (avl_tree_search(index, node, stack)) {
		avl_node_t **inode = STACK_POP(stack);
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

/*
int avl_tree_replace(avl_tree_t *index, avl_node_t *oldnode, avl_node_t *newnode, avl_stack_t *stack) {
	if (index && oldnode && newnode) {
		avl_stack_t ts;
		if (!stack) {
			stack = &ts;
			avl_tree_search(index, oldnode, stack);
		}
		int c;
		for (c = 0; c < stack->top; c++) 
			avl_node_t **inode = stack->nodeptr[c];
			if (*inode == oldnode) {
				(*newnode)->left = (*oldnode)->left;
				(*newnode)->right = (*oldnode)->right;
				(*newnode)->balance = (*oldnode)->balance;
				*inode = newnode;
				return 0;
			}	
		}
	}
	return 1;
}
*/
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
	traverse_left(stack, root);
	if (!iterator->direction && stack->top) {
		stack->nodeptr[0] = stack->nodeptr[stack->top-1];
		stack->top = 1;
	}
}

void avl_iterator_move_last(avl_iterator_t *iterator) {
	avl_stack_t *stack = &iterator->stack;
	stack->top = 0;
	avl_node_t **root = &(iterator->index->root);
	traverse_right(stack, root);
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
			if (iterator->direction) STACK_PUSH(stack,inode); 
			inode = &(*inode)->left;
		} else if (dif<0) {
			if (!iterator->direction) STACK_PUSH(stack,inode);
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
			traverse_left(stack, &(*current)->right);
		else
			traverse_right(stack, &(*current)->left);
		return *current;
	}
	return NULL;
}
