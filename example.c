/**
 * @file example.c
 * @brief Multi-index usage example for the intrusive AVL tree library.
 *
 * Demonstrates how a single `struct person` array can be simultaneously
 * indexed across three independent AVL trees (by ID, full name, and age)
 * without dynamic memory allocation or data duplication.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "avl.h"

/**
 * @brief Sample user payload embedding three intrusive AVL nodes.
 */
struct person {
	unsigned int id;
	char name[16];
	char surname[32];
	unsigned int age;

	/* Intrusive index nodes */
	avl_node_t id_node;
	avl_node_t fullname_node;
	avl_node_t age_node;
};

/** @brief Static sample dataset (zero dynamic allocation). */
struct person sampledata[] = {
	{.id=1, .name="John", .surname="Williams", .age=64},
	{.id=2, .name="Bernetta", .surname="Holt", .age=45},
	{.id=3, .name="Kayden", .surname="Warwick", .age=44},
	{.id=4, .name="Luciana", .surname="Perez", .age=21},
	{.id=5, .name="Morton", .surname="Darby", .age=14},
	{.id=6, .name="Faviano", .surname="Vincenti", .age=57},
	{.id=7, .name="Graham", .surname="Ford", .age=49},
	{.id=8, .name="Belinda", .surname="Williams", .age=65},
	{.id=9, .name="Allannah", .surname="Ford", .age=33},
	{.id=10, .name="John", .surname="Richards", .age=25},
	{.id=11, .name="Cayley", .surname="Erickson", .age=28},
	{.id=12, .name="Emmet", .surname="Taylor", .age=50},
	{.id=13, .name="Liza", .surname="Alesio", .age=23},
	{.id=14, .name="Serenity", .surname="Williams", .age=44},
	{.id=15, .name="Liberty", .surname="Mutton", .age=46},
	{.id=16, .name="Charles", .surname="Danniel", .age=45},
	{.id=17, .name="Francois", .surname="Leclerc", .age=31},
	{.id=18, .name="Carlos", .surname="Garcia", .age=60},
	{.id=19, .name="Donald", .surname="Ford", .age=76},
	{.id=20, .name="John", .surname="Alesio", .age=28}
};

/**
 * @brief Compares two records by unique numeric ID.
 */
int id_compare(avl_node_t *a, avl_node_t *b) {
	struct person *person_a = avl_container_of(a, struct person, id_node);
	struct person *person_b = avl_container_of(b, struct person, id_node);
	return avl_compare(person_a->id, person_b->id);
}

/**
 * @brief Compares two records lexicographically by surname, then given name.
 *
 * The combination of surname + name acts as a composite unique key.
 */
int fullname_compare(avl_node_t *a, avl_node_t *b) {
	struct person *person_a = avl_container_of(a, struct person, fullname_node);
	struct person *person_b = avl_container_of(b, struct person, fullname_node);
	int dif = strncmp(person_a->surname, person_b->surname, 32);
	if (dif == 0) dif = strncmp(person_a->name, person_b->name, 16);
	return dif;
}

/**
 * @brief Compares two records by age, using unique ID as a tie-breaker.
 *
 * Uses `avl_compare()` instead of direct subtraction to avoid signed integer
 * overflow on large unsigned values.
 */
int age_compare(avl_node_t *a, avl_node_t *b) {
	struct person *person_a = avl_container_of(a, struct person, age_node);
	struct person *person_b = avl_container_of(b, struct person, age_node);
	int dif = avl_compare(person_a->age, person_b->age);
	if (dif == 0) dif = avl_compare(person_a->id, person_b->id);
	return dif;
}

/* Global index trees */
avl_tree_t id_tree = avl_tree_init(id_compare);
avl_tree_t fullname_tree = avl_tree_init(fullname_compare);
avl_tree_t age_tree = avl_tree_init(age_compare);

/**
 * @brief Validates uniqueness and inserts a `person` record into all three trees.
 *
 * Reuses the search stack (`id_stack`, `fullname_stack`) so insertion does not
 * need to traverse the tree a second time.
 *
 * @param p Pointer to the record to index.
 * @return `0` on success, `1` if a duplicate ID or full name is detected.
 */
int add_record(struct person *p) {
	avl_stack_t id_stack;
	avl_stack_t fullname_stack;
	void *ret = avl_tree_search(&id_tree, &(p->id_node), &id_stack);
	if (ret) 
		printf("ID not unique\n");
	else {
		ret = avl_tree_search(&fullname_tree, &(p->fullname_node), &fullname_stack);
		if (ret)
			printf("FULLNAME (name+surname) not unique\n");
		else {
			/* Insert into unique trees reusing the recorded search paths */
			avl_tree_insert(&id_tree, &(p->id_node), &id_stack);
			avl_tree_insert(&fullname_tree, &(p->fullname_node), &fullname_stack);
			/* age_tree uses ID as a tie-breaker, so uniqueness is already guaranteed */
			avl_tree_insert(&age_tree, &(p->age_node), NULL);
		}
	}
	return (ret ? 1 : 0);
}

/**
 * @brief Iterates through `tree` in ascending order and prints each record.
 *
 * @param tree   Target index tree to traverse.
 * @param offset Byte offset of the `avl_node_t` member inside `struct person`.
 */
void iterate(avl_tree_t *tree, int offset) {
	avl_iterator_t iter = {.index = tree, .direction = AVL_ITERATOR_RIGHT, .stack = {.top = 0}};
	avl_iterator_move_first(&iter);
	while (avl_iterator_have_data(&iter)) {
		avl_node_t *node = avl_iterator_get(&iter);
		struct person *p = avl_container_in(node, struct person, offset);
		printf("%3d %-16s %-32s %d yo.\n", p->id, p->name, p->surname, p->age); 
	}
}

int main(void) {
	int c;
	for (c = 0; c < 20; c++) add_record(&sampledata[c]);

	printf("Records ordered by ID\n");
	iterate(&id_tree, offsetof(struct person, id_node));

	printf("\nRecords ordered by Fullname\n");
	iterate(&fullname_tree, offsetof(struct person, fullname_node));

	printf("\nRecords ordered by Age\n");
	iterate(&age_tree, offsetof(struct person, age_node));

	return 0;
}
