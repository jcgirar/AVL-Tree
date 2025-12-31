#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "avl.h"

struct person {
	avl_node_t id_node;
	unsigned int id;
	avl_node_t fullname_node;
	char name[16];
	char surname[32];
	avl_node_t age_node;
	unsigned int age;
};
/* just a few invented people to show how the lib works */
struct person sampledata[20] = {
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

avl_tree_t id_tree;
avl_tree_t fullname_tree;
avl_tree_t age_tree;

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
			/* after checking if ID and FULLNAME are uniques do insert */
			avl_tree_insert(&id_tree, &(p->id_node), &id_stack);
			avl_tree_insert(&fullname_tree, &(p->fullname_node), &fullname_stack);
			/* age_tree uses id_tree to compare equal ages so is not necesary check if its unique */
			avl_tree_insert(&age_tree, &(p->age_node), NULL);
		}
	}
	/* if ID or FULLNAME are repeated return ERROR(1) else NOERROR(0) */
	return (ret ? 1 : 0);
}

void fill_trees() {
	int c;
	for (c=0; c<20; c++) add_record(&sampledata[c]);		
}

void iterate_id() {
	avl_iterator_t iter;
	avl_iterator_init(&iter, &id_tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move_first(&iter);
	while(avl_iterator_have_data(&iter)) {
		avl_node_t *node = avl_iterator_get(&iter);
		struct person *p = avl_container_of(node, struct person, id_node);
		printf("%3d %-16s %-32s %d yo.\n", p->id, p->name, p->surname, p->age); 
	}
}

void iterate_fullname() {
	avl_iterator_t iter;
	avl_iterator_init(&iter, &fullname_tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move_first(&iter);
	while(avl_iterator_have_data(&iter)) {
		avl_node_t *node = avl_iterator_get(&iter);
		struct person *p = avl_container_of(node, struct person, fullname_node);
		printf("%3d %-16s %-32s %d yo.\n", p->id, p->name, p->surname, p->age); 
	}
}

void iterate_age() {
	avl_iterator_t iter;
	avl_iterator_init(&iter, &age_tree, AVL_ITERATOR_RIGHT);
	avl_iterator_move_first(&iter);
	while(avl_iterator_have_data(&iter)) {
		avl_node_t *node = avl_iterator_get(&iter);
		struct person *p = avl_container_of(node, struct person, age_node);
		printf("%3d %-16s %-32s %d yo.\n", p->id, p->name, p->surname, p->age); 
	}
}

int fullname_compare(avl_node_t *a, avl_node_t *b) {
	struct person *person_a = avl_container_of(a, struct person, fullname_node);
	struct person *person_b = avl_container_of(b, struct person, fullname_node);
	/* first compare surname and if both have the same then compare name.
	 * The important thing is that combination of both are unique. This makes
	 * that if you iterate the names are ordered by surname and if equals then by name */
	int dif = strncmp(person_a->surname, person_b->surname, 32);
	if (dif == 0) dif = strncmp(person_a->name, person_b->name, 16);
	return dif;
}

int age_compare(avl_node_t *a, avl_node_t *b) {
	struct person *person_a = avl_container_of(a, struct person, age_node);
	struct person *person_b = avl_container_of(b, struct person, age_node);
	/* if ages are in the 'normal' range just substraction its enougth but
	 * if ages are > INT_MAX then a double comparation its necesary because 
	 * int overflow */
	int dif = person_a->age - person_b->age;
	/* if ages are the same the use unique ID to diferenciate */
	if (dif == 0) dif = ((person_a->id < person_b->id) ? -1 : (person_a->id > person_b->id));
	return dif;
}
int main() {
	avl_tree_init(&id_tree, NULL);
	avl_tree_init(&fullname_tree, fullname_compare);
	avl_tree_init(&age_tree, age_compare);
	fill_trees();
	printf("Records ordered by ID\n");
	iterate_id();
	printf("Records ordered by Fullname\n");
	iterate_fullname();
	printf("Records ordered by age\n");
	iterate_age();
	return 0;
}
