/**
 * @file test.c
 * @brief Automated unit tests and AVL invariant verification suite.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "avl.h"

#define TEST_COUNT 500

struct item {
	int key;
	avl_node_t node;
};

static int item_compare(avl_node_t *a, avl_node_t *b) {
	struct item *ia = avl_container_of(a, struct item, node);
	struct item *ib = avl_container_of(b, struct item, node);
	return avl_compare(ia->key, ib->key);
}

/* Recursively verifies AVL height, balance factor, and BST ordering invariants */
static int verify_avl_invariants(avl_tree_t *tree, avl_node_t *node) {
	if (!node) return 0;

	if (node->left) {
		assert(tree->compare(node->left, node) < 0);
	}
	if (node->right) {
		assert(tree->compare(node->right, node) > 0);
	}

	int lh = verify_avl_invariants(tree, node->left);
	int rh = verify_avl_invariants(tree, node->right);
	int expected_balance = rh - lh;

	/* Verify stored balance matches actual subtree heights */
	assert(node->balance == expected_balance);
	assert(node->balance == avl_node_get_balance(node));
	assert(node->balance >= -1 && node->balance <= 1);

	int expected_height = 1 + (lh > rh ? lh : rh);
	assert(avl_node_get_height(node) == expected_height);

	return expected_height;
}

static void test_insert_and_iterators(void) {
	avl_tree_t tree = avl_tree_init(item_compare);
	struct item items[TEST_COUNT];

	/* Insert sequential keys (forces worst-case left/right rotations) */
	for (int i = 0; i < TEST_COUNT; i++) {
		items[i].key = i;
		assert(avl_tree_insert(&tree, &items[i].node, NULL) == 0);
		verify_avl_invariants(&tree, tree.root);
	}

	/* Duplicate insertion must fail */
	struct item dup = { .key = 42 };
	assert(avl_tree_insert(&tree, &dup.node, NULL) == 1);

	/* Forward iteration (ascending) */
	avl_iterator_t iter;
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move_first(&iter);
	int expected = 0;
	while (avl_iterator_have_data(&iter)) {
		avl_node_t *n = avl_iterator_get(&iter);
		struct item *it = avl_container_of(n, struct item, node);
		assert(it->key == expected++);
	}
	assert(expected == TEST_COUNT);

	/* Reverse iteration (descending) */
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_LEFT);
	avl_iterator_move_last(&iter);
	expected = TEST_COUNT - 1;
	while (avl_iterator_have_data(&iter)) {
		avl_node_t *n = avl_iterator_get(&iter);
		struct item *it = avl_container_of(n, struct item, node);
		assert(it->key == expected--);
	}
	assert(expected == -1);

	/* Seek to exact key */
	struct item target = { .key = 250 };
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move(&iter, &target.node);
	assert(avl_iterator_have_data(&iter));
	struct item *found = avl_container_of(avl_iterator_get(&iter), struct item, node);
	assert(found->key == 250);
}

static void test_removals(void) {
	avl_tree_t tree = avl_tree_init(item_compare);
	struct item items[TEST_COUNT];

	/* Pseudo-random insertion order */
	srand(12345);
	for (int i = 0; i < TEST_COUNT; i++) {
		items[i].key = i;
	}
	for (int i = TEST_COUNT - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int tmp = items[i].key;
		items[i].key = items[j].key;
		items[j].key = tmp;
	}

	for (int i = 0; i < TEST_COUNT; i++) {
		assert(avl_tree_insert(&tree, &items[i].node, NULL) == 0);
	}
	verify_avl_invariants(&tree, tree.root);

	/* Remove all elements one by one (exercises all 4 removal cases) */
	for (int i = 0; i < TEST_COUNT; i++) {
		assert(avl_tree_remove(&tree, &items[i].node, NULL) == 0);
		verify_avl_invariants(&tree, tree.root);
	}
	assert(tree.root == NULL);

	/* Removing from an empty tree must fail cleanly */
	assert(avl_tree_remove(&tree, &items[0].node, NULL) == 1);
}

int main(void) {
	test_insert_and_iterators();
	test_removals();
	printf("All AVL tests passed successfully!\n");
	return 0;
}
