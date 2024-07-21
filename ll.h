#pragma once

#include <stdbool.h>
#include "short_types.h"


typedef void *ll_get_key(void *entry);
typedef bool ll_key_equiv(void *k1, void *k2);
typedef void ll_free_entry(void *entry);

typedef struct ll_node ll_node;
typedef struct linkedlist linkedlist;

struct ll_node {
	void *entry;
	ll_node *next;
	ll_node *prev;
};

struct linkedlist {
	ll_node *head;	// leading sentinel
	ll_node *tail;	// ending sentinel
	ll_node *point;	// primarily for the text buffer in struct tab display.c
	ll_get_key *get_key;
	ll_key_equiv *key_equiv;
	ll_free_entry *free_entry;
	u32 length;
};

typedef linkedlist* linkedlist_t;

linkedlist_t ll_new(ll_get_key *get_key, ll_key_equiv *key_equiv,
			ll_free_entry *free_entry);
void ll_free(linkedlist_t l);

void ll_add(linkedlist_t l, void *entry);
void ll_del(linkedlist_t l, void *k);

bool ll_empty(linkedlist_t l);

void *ll_get(linkedlist_t l, void *k);
void *ll_get_head(linkedlist_t l);

/* Functions for manipulating l->point */
void *ll_point(linkedlist_t l);
bool ll_point_at_start(linkedlist_t l);
bool ll_point_at_end(linkedlist_t l);
void ll_next(linkedlist_t l);
void ll_prev(linkedlist_t l);
