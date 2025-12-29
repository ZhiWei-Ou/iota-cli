/**
 * @brief 通用哈希容器
 * @file xhash.c
 * @author Oswin
 * @date 2025-06-26
 * @details A general-purpose hash table implementation using open addressing
 *           with double hashing for collision resolution.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xhash.h"

#include <stdlib.h>
#include <string.h>

#define xhash_probe(h1, h2, count, cap) ((h1 + count * (h2 | 1)) % cap)

static int8_t dummy = 0xff;

inline static xbool_t xhash_table_is_pow2(size_t n) {
  return (n & (n - 1)) == 0;
}
static err_t xhash_clear_custom(xhash self, xhash_destroy_func destory);
static void* xhash_lookup_with_cmp(xhash self,
                                   const void* data,
                                   xhash_cmp_func cmp);

err_t xhash_init(struct xhash_priv* self,
                 xhash_table* table,
                 size_t table_size,
                 xhash_func h1,
                 xhash_func h2,
                 xhash_cmp_func cmp,
                 xhash_destroy_func destroy) {
  if (self == NULL || table == NULL || table_size <= 0 || h1 == NULL ||
      h2 == NULL || cmp == NULL || destroy == NULL)
    return X_RET_INVAL;

  memset(table, 0, sizeof(xhash_table) * table_size);

  self->length = 0;
  self->table = table;
  self->table_cap = table_size;
  self->h1 = h1;
  self->h2 = h2;
  self->cmp = cmp;
  self->destory = destroy;

  return X_RET_OK;
}

xhash xhash_create(xhash_table* table,
                   size_t table_cap,
                   xhash_func h1,
                   xhash_func h2,
                   xhash_cmp_func match,
                   xhash_destroy_func destory) {
  if (table == NULL || table_cap <= 0 || h1 == NULL || h2 == NULL ||
      match == NULL || destory == NULL)
    return NULL;

  /* clear table */
  memset(table, 0, sizeof(xhash_table) * table_cap);

  xhash self = xbox_malloc(sizeof(struct xhash_priv));
  if (self == NULL) return NULL;

  self->length = 0;
  self->table = table;
  self->table_cap = table_cap;
  self->h1 = h1;
  self->h2 = h2;
  self->cmp = match;
  self->destory = destory;

  return self;
}

err_t xhash_insert(xhash self, void* data) {
  if (self == NULL || data == NULL) return X_RET_INVAL;

  err_t ret = X_RET_ERROR;

  if (self->length == self->table_cap) return X_RET_FULL;

  uint32_t hk1 = self->h1(data);
  uint32_t hk2 = self->h2(data);

  for (size_t i = 0; i < self->table_cap; i++) {
    uint32_t pos = xhash_probe(hk1, hk2, i, self->table_cap);

    if (self->table[pos] == NULL || self->table[pos] == &dummy) {
      self->table[pos] = data;
      self->length++;
      ret = X_RET_OK;
      break;
    }

    if (!self->cmp(self->table[pos], data)) {
      ret = X_RET_EXIST;
      break;
    }
  }

  return ret;
}

static void* xhash_lookup_with_cmp(xhash self,
                                   const void* data,
                                   xhash_cmp_func cmp) {
  if (self == NULL || data == NULL) return NULL;

  void* target = NULL;

  uint32_t hk1 = self->h1(data);
  uint32_t hk2 = self->h2(data);

  for (size_t i = 0; i < self->table_cap; i++) {
    uint32_t pos = xhash_probe(hk1, hk2, i, self->table_cap);

    if (self->table[pos] == NULL) return NULL;

    if (self->table[pos] == &dummy) continue;

    if (!cmp(self->table[pos], data)) {
      target = self->table[pos];
      break;
    }
  }

  return target;
}

void* xhash_lookup(xhash self, const void* data) {
  return xhash_lookup_with_cmp(self, data, self->cmp);
}

err_t xhash_list_data(xhash self,
                      void (*visit)(const void* data, void* closure),
                      void* closure) {
  if (self == NULL || visit == NULL) return X_RET_INVAL;

  for (size_t i = 0; i < self->table_cap; i++) {
    if (self->table[i] == NULL || self->table[i] == &dummy) continue;

    visit(self->table[i], closure);
  }

  return X_RET_OK;
}

int xhash_list_lookup(xhash self,
                      const void* data,
                      void** matchs,
                      size_t matchs_size,
                      xhash_cmp_func cmp) {
  if (self == NULL || data == NULL || matchs == NULL || matchs_size <= 0 ||
      cmp == NULL)
    return X_RET_INVAL;

  int match_count = 0;
  for (size_t i = 0; i < self->table_cap; i++) {
    if (self->table[i] == NULL || self->table[i] == &dummy) continue;

    if (!cmp(self->table[i], data)) {
      matchs[match_count++] = self->table[i];
      continue;
    }

    if (match_count == matchs_size) break;
  }

  return match_count;
}

err_t xhash_remove(xhash self, const void* data) {
  if (self == NULL || data == NULL) return X_RET_INVAL;

  uint32_t hk1 = self->h1(data);
  uint32_t hk2 = self->h2(data);

  for (size_t i = 0; i < self->table_cap; i++) {
    uint32_t pos = xhash_probe(hk1, hk2, i, self->table_cap);

    if (self->table[pos] == NULL) return X_RET_NOTENT;

    if (self->table[pos] == &dummy) continue;

    if (self->cmp(self->table[pos], data) == 0) {
      self->length--;
      self->destory(self->table[pos]);
      self->table[pos] = &dummy;
      return X_RET_OK;
    }
  }

  return X_RET_NOTENT;
}

void* xhash_remove_hold(xhash self, const void* data) {
  if (self == NULL || data == NULL) return NULL;

  uint32_t hk1 = self->h1(data);
  uint32_t hk2 = self->h2(data);

  for (size_t i = 0; i < self->table_cap; i++) {
    uint32_t pos = xhash_probe(hk1, hk2, i, self->table_cap);

    if (self->table[pos] == NULL) return NULL;

    if (self->table[pos] == &dummy) continue;

    if (self->cmp(self->table[pos], data) == 0) {
      void* ret = self->table[pos];
      self->length--;
      self->table[pos] = &dummy;
      return ret;
    }
  }

  return NULL;
}

err_t xhash_clear(xhash self) {
  return xhash_clear_custom(self, self->destory);
}

err_t xhash_clear_custom(xhash self, xhash_destroy_func destory) {
  if (self == NULL) return X_RET_INVAL;

  if (self->length == 0) return X_RET_OK;

  for (size_t i = 0; i < self->table_cap; i++) {
    if (self->table[i] == NULL) continue;

    if (self->table[i] != &dummy) destory(self->table[i]);

    self->table[i] = NULL;
    self->length--;
  }

  return X_RET_OK;
}

size_t xhash_length(xhash self) {
  if (self == NULL) return 0;

  return self->length;
}

size_t xhash_capacity(xhash self) {
  if (self == NULL) return 0;

  return self->table_cap;
}

err_t xhash_destroy(xhash self) {
  if (self == NULL) return X_RET_INVAL;

  xhash_clear(self);

  xbox_free(self);

  return X_RET_OK;
}

err_t xhash_foreach_seek_and_get_nowarning(xhash self,
                                           size_t* idx,
                                           void** data) {
  if (!self || !idx || !data) return X_RET_INVAL;

  for (size_t i = (*idx); i < self->table_cap; i++) {
    if (self->table[i] == NULL || self->table[i] == &dummy) continue;

    *idx = i;
    *data = self->table[i];
    return X_RET_OK;
  }

  return X_RET_NOTENT;
}

uint32_t xhash_djb2(const void* key, size_t len) {
  const unsigned char* str = (const unsigned char*)key;
  uint32_t hash = 5381;

  for (size_t i = 0; i < len; ++i) {
    hash = ((hash << 5) + hash) + str[i];  // hash * 33 + c
  }

  return hash;
}

uint32_t xhash_bkdr(const void* key, size_t len) {
  const unsigned char* str = (const unsigned char*)key;
  uint32_t seed = 131;  // e.g. 31、131、1313、13131 、...
  uint32_t hash = 0;

  for (size_t i = 0; i < len; ++i) {
    hash = hash * seed + str[i];
  }

  return hash;
}

uint32_t xhash_fnv1a(const void* key, size_t len) {
  const unsigned char* data = (const unsigned char*)key;
  uint32_t hash = 2166136261u;       // FNV offset basis
  const uint32_t prime = 16777619u;  // FNV prime

  for (size_t i = 0; i < len; ++i) {
    hash ^= data[i];  // XOR byte into the bottom of the hash
    hash *= prime;    // Multiply by FNV prime
  }

  return hash;
}

uint32_t xhash_next_power_of_two(uint32_t value) {
  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  return value + 1;
}
