/**
 * @brief xhashmap is an @ref xhash that supports automatic expansion
 * @file xhashmap.c
 * @author Oswin
 * @date 2025-12-10
 * @details xhashmap automatically expands its capacity when the load factor reaches 66% to maintain efficient insert operations
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
// #define XBOX_ENABLE_BACKTRACE
#include "xhashmap.h"

#include <string.h>

#include "xhash.h"

typedef struct {
  char* k;
  void* v;
} xhashmap_pair;
static xhashmap_pair* xhashmap_pair_new(const char* k, void* v);
static uint32_t xhashmap_pair_hash1(const void*);
static uint32_t xhashmap_pair_hash2(const void*);
static int xhashmap_pair_cmp(const void*, const void*);
static void xhashmap_pair_free(void* p);

struct xhashmap_priv {
  xhash hash;
  xhash_table* tbl;
  size_t tbl_size;
  xhashmap_destroy_fn destroy;
};

static xbool_t xhashmap_need_grow(xhashmap map);
static void xhashmap_rehash(xhashmap map);

xhashmap xhashmap_create_ex(xhashmap_destroy_fn destroy_nullable,
                            size_t initial_capacity) {
  xhashmap self = (xhashmap)xbox_malloc(sizeof(struct xhashmap_priv));
  if (!self) return NULL;

  if (initial_capacity == 0)
    initial_capacity = X_HASHMAP_DEFAULT_CAPACITY;
  else {
    // next power of 2 (64-bit safe)
    if (sizeof(size_t) == 8) initial_capacity |= initial_capacity >> 32;

    initial_capacity--;
    initial_capacity |= initial_capacity >> 1;
    initial_capacity |= initial_capacity >> 2;
    initial_capacity |= initial_capacity >> 4;
    initial_capacity |= initial_capacity >> 8;
    initial_capacity |= initial_capacity >> 16;
    initial_capacity += 1;
  }

  self->destroy = destroy_nullable;
  self->tbl_size = initial_capacity;
  self->tbl = xbox_malloc(sizeof(xhash_table) * initial_capacity);
  if (!self->tbl) {
    xbox_free(self);
    return NULL;
  }

  self->hash = xhash_create(self->tbl,
                            self->tbl_size,
                            xhashmap_pair_hash1,
                            xhashmap_pair_hash2,
                            xhashmap_pair_cmp,
                            xhashmap_pair_free);
  if (!self->hash) {
    xbox_free(self->tbl);
    xbox_free(self);
    return NULL;
  }

  return self;
}

size_t xhashmap_length(xhashmap map) {
  if (!map) return 0;
  return xhash_length(map->hash);
}

size_t xhashmap_capacity(xhashmap map) {
  if (!map) return 0;
  return xhash_capacity(map->hash);
}

err_t xhashmap_insert(xhashmap map, const char* key, void* data) {
  if (!map || !key || !data) return X_RET_INVAL;

  xhashmap_pair* pair = xhashmap_pair_new(key, data);
  if (!pair) return X_RET_NOMEM;

  err_t err = xhash_insert(map->hash, pair);
  if (err != X_RET_OK) {
    xhashmap_pair_free(pair);
    return err;
  }

  if (xhashmap_need_grow(map)) xhashmap_rehash(map);

  return X_RET_OK;
}

void* xhashmap_lookup(xhashmap map, const char* key) {
  if (!map || !key) return NULL;
  xhashmap_pair t = {(char*)key, NULL};
  xhashmap_pair* f = (xhashmap_pair*)xhash_lookup(map->hash, &t);
  if (!f) return NULL;

  return f->v;
}

int xhashmap_lookup_ex(xhashmap map,
                       const void* key,
                       void** out_data,
                       size_t out_size,
                       int (*cmp_fn)(const void* item, const void* key)) {
  if (!map || !key || !out_data || out_size == 0 || !cmp_fn) return 0;

  int matched = 0;
  xhashmap_pair* pos;
  xhash_foreach(map->hash, pos) {
    if (cmp_fn(pos->v, key) == 0) {
      out_data[matched++] = pos->v;
    }

    if (matched >= out_size) break;
  }

  return matched;
}

err_t xhashmap_remove(xhashmap map, const char* key) {
  if (!map || !key) return X_RET_INVAL;
  xhashmap_pair t = {(char*)key, NULL};
  xhashmap_pair* f = (xhashmap_pair*)xhash_remove_hold(map->hash, &t);
  if (!f) return X_RET_NOTENT;

  if (map->destroy) map->destroy(f->v);
  xhashmap_pair_free(f);
  return X_RET_OK;
}

void* xhashmap_remove_hold(xhashmap map, const char* key) {
  if (!map || !key) return NULL;
  xhashmap_pair t = {(char*)key, NULL};
  xhashmap_pair* f = (xhashmap_pair*)xhash_remove_hold(map->hash, &t);
  if (!f) return NULL;

  void* ret = f->v;
  xhashmap_pair_free(f);
  return ret;
}

err_t xhashmap_clear(xhashmap map) {
  if (!map) return X_RET_INVAL;

  xhashmap_pair* pos;
  xhash_foreach(map->hash, pos) {
    if (map->destroy) map->destroy(pos->v);
    xhashmap_pair_free(pos);
    map->hash->table[___i] = NULL;
  }

  map->hash->length = 0;

  return X_RET_OK;
}

err_t xhashmap_destroy(xhashmap map) {
  if (!map) return X_RET_INVAL;

  xhashmap_clear(map);
  xhash_destroy(map->hash);
  xbox_free(map->tbl);
  xbox_free(map);

  return X_RET_OK;
}

static xhashmap_pair* xhashmap_pair_new(const char* k, void* v) {
  xhashmap_pair* p = (xhashmap_pair*)xbox_malloc(sizeof(xhashmap_pair));
  if (!p) return NULL;
  p->k = xbox_strdup(k);
  p->v = v;
  return p;
}

static uint32_t xhashmap_pair_hash1(const void* pair) {
  xhashmap_pair* p = (xhashmap_pair*)pair;
  return xhash_bkdr(p->k, strlen(p->k));
}

static uint32_t xhashmap_pair_hash2(const void* pair) {
  xhashmap_pair* p = (xhashmap_pair*)pair;
  return xhash_fnv1a(p->k, strlen(p->k));
}

static int xhashmap_pair_cmp(const void* p1, const void* p2) {
  xhashmap_pair* t1 = (xhashmap_pair*)p1;
  xhashmap_pair* t2 = (xhashmap_pair*)p2;
  return strcmp(t1->k, t2->k);
}

static void xhashmap_pair_free(void* p) {
  xhashmap_pair* pair = (xhashmap_pair*)p;
  xbox_free(pair->k);
  xbox_free(pair);
}

static xbool_t xhashmap_need_grow(xhashmap map) {
  return xhash_length(map->hash) >= map->tbl_size * 2 / 3;
}
static void xhashmap_rehash(xhashmap map) {
  size_t new_size = map->tbl_size * 2;
  xhash_table* new_tbl = xbox_malloc(sizeof(xhash_table) * new_size);
  if (!new_tbl) return;

  xhash new_hash = xhash_create(new_tbl,
                                new_size,
                                xhashmap_pair_hash1,
                                xhashmap_pair_hash2,
                                xhashmap_pair_cmp,
                                xhashmap_pair_free);
  if (!new_hash) {
    xbox_free(new_tbl);
    return;
  }

  struct xhashmap_pair* pos;
  xhash_foreach(map->hash, pos) {
    // xhash_remove_hold(map->hash, pos);
    map->hash->table[___i] = NULL;

    xhash_insert(new_hash, pos);
  }
  map->hash->length = 0;

  xhash_destroy(map->hash);

  map->hash = new_hash;
  map->tbl_size = new_size;
  xbox_free(map->tbl);
  map->tbl = new_tbl;

  return;
}
