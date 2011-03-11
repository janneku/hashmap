/*
 * hashmap.c -- generic hash table
 *
 * Copyright (c) 2011, Janne Kulmala <janne.t.kulmala@tut.fi>.
 * All rights reserved.
 *
 * See LICENSE for the license.
 *
 */

#include "hashmap.h"
#include <stdlib.h>

#define MIN_SLOTS	16

static int hashmap_grow(struct hashmap *map)
{
	size_t i;

	/* first, allocate more room for the table */
	struct hash_node **newtable = realloc(map->table, map->len * 2 *
					      sizeof(struct hash_node *));
	if (newtable == NULL)
		return -1;
	map->table = newtable;

	/* then, split all nodes from the lower half of the table
	   to either lower or upper half of the table */
	for (i = 0; i < map->len; ++i) {
		struct hash_node *node = map->table[i], *next;
		struct hash_node *a = NULL, *b = NULL;
		while (node) {
			next = node->next;
			if (node->hash & map->len) {
				/* upper half */
				node->next = b;
				b = node;
			} else {
				/* lower half */
				node->next = a;
				a = node;
			}
			node = next;
		}
		map->table[i] = a;
		map->table[i + map->len] = b;
	}
	map->len *= 2;
	return 0;
}

static int hashmap_shrink(struct hashmap *map)
{
	size_t i;

	/* first, fold the upper half of the table to top of the lower half */
	map->len /= 2;
	for (i = 0; i < map->len; ++i) {
		struct hash_node *prev = map->table[i];
		struct hash_node *next = map->table[i + map->len];
		if (prev == NULL)
			map->table[i] = next;
		else {
			while (prev->next)
				prev = prev->next;
			prev->next = next;
		}
	}
	/* then, release unneeded memory */
	struct hash_node **newtable = realloc(map->table, map->len *
					      sizeof(struct hash_node *));
	if (newtable == NULL)
		return -1;
	map->table = newtable;
	return 0;
}

void hashmap_init(struct hashmap *map, hash_func_t hash, cmp_func_t cmp)
{
	map->len = MIN_SLOTS;
	map->table = calloc(map->len, sizeof(struct hash_node *));
	map->count = 0;
	map->hash = hash;
	map->cmp = cmp;
}

void hashmap_free(struct hashmap *map)
{
	free(map->table);
}

struct hash_node *hashmap_get(struct hashmap *map, void *key)
{
	struct hash_node *node = map->table[map->hash(key) & (map->len - 1)];
	while (node) {
		if (map->cmp(node, key))
			return node;
		node = node->next;
	}
	return NULL;
}

int hashmap_insert(struct hashmap *map, struct hash_node *node, void *key)
{
	size_t slot;
	node->hash = map->hash(key);
	slot = node->hash & (map->len - 1);
	node->next = map->table[slot];
	map->table[slot] = node;
	map->count++;

	if (map->count > map->len * 3)
		hashmap_grow(map);
	return 0;
}

struct hash_node *hashmap_remove(struct hashmap *map, void *key)
{
	size_t slot = map->hash(key) & (map->len - 1);
	struct hash_node *node = map->table[slot], *prev = NULL;
	while (node) {
		if (map->cmp(node, key)) {
			if (prev != NULL)
				prev->next = node->next;
			else
				map->table[slot] = node->next;
			map->count--;

			if (map->count < map->len / 4 && map->len > MIN_SLOTS)
				hashmap_shrink(map);
			return node;
		}
		prev = node;
		node = node->next;
	}
	return NULL;
}

#ifdef TEST

#include <stdio.h>
#include <assert.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) \
	((type *) ((char *) (ptr) - offsetof(type, member)))

struct test {
	struct hash_node node;
	int i, j;
};

static size_t hash_test(void *key)
{
	int *i = key;
	return *i;
}

static int cmp_test(struct hash_node *node, void *key)
{
	struct test *t = container_of(node, struct test, node);
	int *i = key;
	return t->i == *i;
}

#define COUNT		1000000
#define GET_COUNT	10000000

int main()
{
	int i;
	struct test *t;
	struct hashmap map;

	hashmap_init(&map, hash_test, cmp_test);

	for (i = 0; i < COUNT; ++i) {
		t = calloc(1, sizeof *t);
		t->i = i;
		t->j = i + 123;

		hashmap_insert(&map, &t->node, &i);
	}

	for (i = 0; i < GET_COUNT; ++i) {
		int k = rand() % COUNT;
		struct test *t;
		struct hash_node *node = hashmap_get(&map, &k);
		if (node == NULL) {
			printf("%d not found\n", k);
			assert(0);
		}
		t = container_of(node, struct test, node);
		assert (t->i == k && t->j == k + 123);
	}

	for (i = 0; i < COUNT; ++i) {
		int k = COUNT - 1 - i;
		struct hash_node *node = hashmap_remove(&map, &k);
		if (node == NULL) {
			printf("%d not found\n", k);
			assert(0);
		}
		t = container_of(node, struct test, node);;
		assert (t->i == k && t->j == k + 123);
	}

	assert(map.len == MIN_SLOTS);

	return 0;
}

#endif
