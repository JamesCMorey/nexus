#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ll.h"


typedef struct box {
	int *label;
	int value;
} box;

bool key_equiv(void *k1, void *k2) {
	return *(int*)k1 == *(int*)k2;
}

void *get_key(void *e) {
	return (void*)((box*)e)->label;
}

void free_entry(void *e) {
	free(((box*)e)->label);
	free(e);
}

box *box_new(int x) {
	box *tmp = malloc(sizeof(box));
	tmp->label = malloc(sizeof(int));
	*tmp->label = x;
	tmp->value = x;
	return tmp;
}

int main() { // minimal testing to catch major issues. No real edgecases
	linkedlist_t test = ll_new(&get_key, &key_equiv, &free_entry);

	int *tmp;
	box *r1 = box_new(1);
	box *r2 = box_new(2);
	box *r2_2 = box_new(2);
	box *r3 = box_new(3);
	box *r4 = box_new(4);
	box *r5 = box_new(5);
	box *r6 = box_new(6);

	/* Testing ll_add (kinda) */
	ll_add(test, r1);
	ll_add(test, r2);
	ll_add(test, r3);
	ll_add(test, r4);
	ll_add(test, r5);

	/* Testing ll_get */
	tmp = ll_get(test, get_key(r2));
	assert(tmp != NULL && (box*)tmp == r2);

	tmp = ll_get(test, get_key(r2_2));
	assert(tmp != NULL && (box*)tmp == r2);

	tmp = ll_get(test, r6);
	assert(tmp == NULL);

	/* Testing ll_del*/
	ll_del(test, get_key(r2));
	assert(ll_get(test, get_key(r2_2)) == NULL);

	ll_free(test);
	free(r6->label);
	free(r6);
	free(r2_2->label);
	free(r2_2);

	printf("All tests passed.\n");
	return 0;
}
