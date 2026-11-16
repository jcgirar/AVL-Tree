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
static void test_find_and_null_safety(void) {
	avl_tree_t tree = avl_tree_init(item_compare);
	avl_tree_t uninit_tree = { .compare = NULL, .root = NULL };
	struct item items[10];
	struct item missing = { .key = 999 };
	avl_stack_t stack = { .top = 0 };
	avl_iterator_t iter = { .direction = AVL_ITERATOR_RIGHT, .index = NULL, .stack = { .top = 0 } };

	/* Populate small tree with even keys: 0, 2, 4, ..., 18 */
	for (int i = 0; i < 10; i++) {
		items[i].key = i * 2;
		assert(avl_tree_insert(&tree, &items[i].node, NULL) == 0);
	}

	/* 1. Verify avl_tree_find() for present and absent keys */
	for (int i = 0; i < 10; i++) {
		struct item query = { .key = i * 2 };
		assert(avl_tree_find(&tree, &query.node) == &items[i].node);
	}
	assert(avl_tree_find(&tree, &missing.node) == NULL);

	/* 2. Null safety on tree queries and mutations */
	assert(avl_tree_find(NULL, &items[0].node) == NULL);
	assert(avl_tree_find(&tree, NULL) == NULL);
	assert(avl_tree_find(&uninit_tree, &items[0].node) == NULL);

	assert(avl_tree_search(NULL, &items[0].node, &stack) == NULL);
	assert(avl_tree_search(&tree, NULL, &stack) == NULL);
	assert(avl_tree_search(&tree, &items[0].node, NULL) == NULL);
	assert(avl_tree_search(&uninit_tree, &items[0].node, &stack) == NULL);

	assert(avl_tree_insert(NULL, &missing.node, NULL) == 1);
	assert(avl_tree_insert(&tree, NULL, NULL) == 1);
	assert(avl_tree_insert(&uninit_tree, &missing.node, NULL) == 1);

	assert(avl_tree_remove(NULL, &items[0].node, NULL) == 1);
	assert(avl_tree_remove(&tree, NULL, NULL) == 1);
	assert(avl_tree_remove(&uninit_tree, &items[0].node, NULL) == 1);

	/* 3. Null safety on iterators */
	avl_iterator_init(NULL, &tree, AVL_ITERATOR_RIGHT);
	avl_iterator_init(&iter, NULL, AVL_ITERATOR_RIGHT);
	assert(avl_iterator_have_data(NULL) == 0);
	assert(avl_iterator_have_data(&iter) == 0);

	avl_iterator_move_first(NULL);
	avl_iterator_move_first(&iter);
	avl_iterator_move_last(NULL);
	avl_iterator_move_last(&iter);
	avl_iterator_move(NULL, &items[0].node);
	avl_iterator_move(&iter, &items[0].node);
	avl_iterator_move(&iter, NULL);

	assert(avl_iterator_get(NULL) == NULL);
	assert(avl_iterator_get(&iter) == NULL);

	verify_avl_invariants(&tree, tree.root);
}

static void test_iterator_bounds(void) {
	avl_tree_t tree = avl_tree_init(item_compare);
	struct item items[10];
	avl_iterator_t iter;

	/* Insert even keys: 10, 20, 30, ..., 100 */
	for (int i = 0; i < 10; i++) {
		items[i].key = (i + 1) * 10;
		assert(avl_tree_insert(&tree, &items[i].node, NULL) == 0);
	}

	/* 1. Seek missing intermediate key (45) ascending -> should yield 50, 60, ..., 100 */
	struct item seek_mid = { .key = 45 };
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move(&iter, &seek_mid.node);
	int expected = 50;
	while (avl_iterator_have_data(&iter)) {
		struct item *it = avl_container_of(avl_iterator_get(&iter), struct item, node);
		assert(it->key == expected);
		expected += 10;
	}
	assert(expected == 110);

	/* 2. Seek missing intermediate key (45) descending -> should yield 40, 30, 20, 10 */
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_LEFT);
	avl_iterator_move(&iter, &seek_mid.node);
	expected = 40;
	while (avl_iterator_have_data(&iter)) {
		struct item *it = avl_container_of(avl_iterator_get(&iter), struct item, node);
		assert(it->key == expected);
		expected -= 10;
	}
	assert(expected == 0);

	/* 3. Seek key past maximum (999) ascending -> empty iterator */
	struct item seek_high = { .key = 999 };
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move(&iter, &seek_high.node);
	assert(avl_iterator_have_data(&iter) == 0);
	assert(avl_iterator_get(&iter) == NULL);

	/* 4. Seek key below minimum (5) descending -> empty iterator */
	struct item seek_low = { .key = 5 };
	avl_iterator_init(&iter, &tree, AVL_ITERATOR_LEFT);
	avl_iterator_move(&iter, &seek_low.node);
	assert(avl_iterator_have_data(&iter) == 0);
	assert(avl_iterator_get(&iter) == NULL);
}

int main(void) {
	test_insert_and_iterators();
	test_removals();
	test_find_and_null_safety();
	test_iterator_bounds();
	printf("All AVL tests passed successfully!\n");
	return 0;
}
