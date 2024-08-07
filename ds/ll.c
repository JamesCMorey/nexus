#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "../log.h"
#include "ll.h"
#include "short_types.h"

// can't be bothered to check for circularity
bool is_ll(linkedlist *l) {
	if (l == NULL || l->head == NULL || l->tail == NULL)
		return false;
	if (l->key_equiv == NULL || l->get_key == NULL) // free_entry optional
		return false;

	ll_node *tmp = l->head->next;
	ll_node *prev = l->head;
	while (tmp != l->tail) {
		if (prev != tmp->prev)
			return false;
		prev = tmp;
		tmp = tmp->next;
	}

	return true;
}

linkedlist *ll_new(ll_get_key *get_key, ll_key_equiv *key_equiv,
			ll_free_entry *free_entry) {
	linkedlist *l = malloc(sizeof(linkedlist));

	l->head = malloc(sizeof(ll_node));
	l->tail = malloc(sizeof(ll_node));
	l->head->next = l->tail;
	l->tail->prev = l->head;

	l->key_equiv = key_equiv;
	l->get_key = get_key;
	l->free_entry = free_entry;

	assert(is_ll(l));
	return l;
}

/* Adds to head of list*/
void ll_add(linkedlist *l, void *entry) {
	assert(is_ll(l) && entry != NULL);

	ll_node *tmp = malloc(sizeof(ll_node));
	tmp->entry = entry;
	tmp->prev = l->head;
	tmp->next = l->head->next;

	l->head->next->prev = tmp;
	l->head->next = tmp;

	l->point = tmp; // update point so its at first elem

	l->length++;
	assert(is_ll(l));
}

/* frees both node in list and entry in node */
void ll_del(linkedlist *l, void *k) {
	assert(is_ll(l) && k != NULL);

	ll_node *tmp = l->head->next;
	while (tmp != l->tail) {
		if (l->key_equiv(k, l->get_key(tmp->entry))) {
			tmp->prev->next = tmp->next;
			tmp->next->prev = tmp->prev;

			if (l->free_entry != NULL)
				l->free_entry(tmp->entry);
			free(tmp);
			l->length--;
			assert(is_ll(l));
			return;
		}
		tmp = tmp->next;
	}
	assert(is_ll(l));
}

void ll_free(linkedlist *l) {
	assert(is_ll(l));

	if (ll_empty(l)) {
		free(l->head);
		free(l->tail);
		return;
	}


	ll_node *tmp = l->head->next;
	ll_node *freer;
	free(l->head);
	while (tmp->next != l->tail) {
		freer = tmp;
		tmp = tmp->next;
		if (l->free_entry != NULL)
			l->free_entry(freer->entry);
		free(freer);
	}

	if (l->free_entry != NULL)
		l->free_entry(tmp->entry);
	free(tmp);
	free(l->tail);
	free(l);
}

ll_node *ll_get_node(linkedlist *l, void *k) {
	assert(is_ll(l) && k != NULL);
	ll_node *tmp = l->head->next;
	while (tmp != l->tail) {
		if (l->key_equiv(k, l->get_key(tmp->entry))) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;

}

void *ll_get(linkedlist *l, void *k) {
	assert(is_ll(l) && k != NULL);
	ll_node *tmp = ll_get_node(l, k);

	if (tmp != NULL)
		return tmp->entry;

	return NULL;
}

bool ll_empty(linkedlist *l) {
	assert(is_ll(l));
	return l->head->next == l->tail;
}

// returns l->head->next in reality
void *ll_get_head(linkedlist *l)  {
	assert(is_ll(l) && !ll_empty(l));
	return l->head->next->entry;
}

/* Functions for manipulating l->point */

void ll_reset_point(linkedlist *l) {
	l->point = l->head->next;
}
void ll_set_point(linkedlist *l, void *k) {
	wlog("trying to get tab (%d)", *(int*)k);

	l->point = ll_get_node(l, k);
	l->point == NULL ? wlog("setpoint failed") : wlog("setpoint success");
}

void *ll_point(linkedlist *l) {
	assert(is_ll(l) && !ll_empty(l) && l->point != NULL);
	return l->point->entry;
}

bool ll_point_at_head(linkedlist *l) {
	assert(is_ll(l) && !ll_empty(l));
	return l->point->prev == l->head;
}

bool ll_point_at_tail(linkedlist *l) {
	assert(is_ll(l) && !ll_empty(l));
	return l->point->next == l->tail;
}

void ll_next(linkedlist *l) {
	assert(is_ll(l) && !ll_point_at_tail(l));
	l->point = l->point->next;
}

void ll_prev(linkedlist *l) {
	assert(is_ll(l) && !ll_point_at_head(l));
	l->point = l->point->prev;
}
