/*
 * hashmap.h -- generic hash table
 *
 * Copyright (c) 2010, Janne Kulmala <janne.t.kulmala@tut.fi>.
 * All rights reserved.
 *
 * See LICENSE.hashmap for the license.
 *
 */

#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#include <string.h>

struct hash_node {
	size_t hash;
	struct hash_node *next;
};

typedef size_t (*hash_func_t)(void *key);
typedef int (*cmp_func_t)(struct hash_node *node, void *key);

struct hashmap {
	struct hash_node **table;
	size_t len, count;
	hash_func_t hash;
	cmp_func_t cmp;
};

void hashmap_init(struct hashmap *map, hash_func_t hash, cmp_func_t cmp);
void hashmap_free(struct hashmap *map);
struct hash_node *hashmap_get(struct hashmap *map, void *key);
int hashmap_insert(struct hashmap *map, struct hash_node *node, void *key);
struct hash_node *hashmap_remove(struct hashmap *map, void *key);

#endif
