#ifndef AVL_H_INCLUDED
#define AVL_H_INCLUDED 1

#include <stddef.h>
#include <stdint.h>

/* utility macro because avl_node_t must always be included in other structures
 * and this macro is used to get that container structure  */
#define avl_container_of(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))
/* utility macro used to get the uint key just after the avl_node_t */
#define AVL_UINTKEY(ptr) *((unsigned int *)((char *)(ptr)+sizeof(avl_node_t)))


typedef struct avl_node avl_node_t;
struct avl_node {
    avl_node_t *left;
    avl_node_t *right;
    int balance;
};

#define AVL_STACK_SIZE 32
#define STACK_PUSH(stack,value) (stack)->nodeptr[(stack)->top++] = (value)
#define STACK_POP(stack) (stack)->nodeptr[--(stack)->top]

typedef struct avl_stack avl_stack_t;
struct avl_stack {
	avl_node_t **nodeptr[AVL_STACK_SIZE];
	unsigned int top;
};


typedef struct avl_tree avl_tree_t;
struct avl_tree {
	int (*compare)(avl_node_t *, avl_node_t *);
	avl_node_t *(*search)(avl_tree_t *, avl_node_t *, avl_stack_t *);
	avl_node_t *root;
};

#define AVL_ITERATOR_LEFT 0
#define AVL_ITERATOR_RIGHT 1

typedef struct {
	int direction;
	avl_tree_t *index;
	avl_stack_t stack;
} avl_iterator_t;

int 			avl_node_get_height(avl_node_t *);
int				avl_tree_init(avl_tree_t *, int (*)(avl_node_t *, avl_node_t *));
avl_node_t *	avl_tree_search(avl_tree_t *, avl_node_t *, avl_stack_t *);
int 			avl_tree_insert(avl_tree_t *, avl_node_t *, avl_stack_t *);
int 			avl_tree_remove(avl_tree_t *, avl_node_t *);
void 			avl_iterator_init(avl_iterator_t *, avl_tree_t *, int);
int 			avl_iterator_have_data(avl_iterator_t *);
void 			avl_iterator_move_first(avl_iterator_t *); 
void 			avl_iterator_move_last(avl_iterator_t *);
void 			avl_iterator_move(avl_iterator_t *, avl_node_t *); 
avl_node_t *	avl_iterator_get(avl_iterator_t *);

#endif // AVL_H_INCLUDED 
