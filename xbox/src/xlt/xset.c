/**
 * @brief 跳表算法容器
 * @file xset.c
 * @author Oswin
 * @date 2025-07-04
 * @details
 * @ref https://en.wikipedia.org/wiki/Skip_list
 * @graph
 * --> 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10
 *        ^   ^   ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |   |   |
 * -->    1   2   3   4   5   6   7   8   9   10
 *        |       |       |       |       |   |
 * -->    1       3       5       7       9   10
 *        |                       |
 * -->    1                       7
 *        |
 * -->    1
 * @endgraph
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xset.h"

#include <stdlib.h>

#define XSET_MAX_DEPTH (32)
#define XSET_PROBABILITY (0.25)

typedef struct xset_node_ {
  void* data;                  /* 数据 */
  struct xset_node_* backward; /* 后继节点 */
  struct xset_node_level {
    struct xset_node_* forward; /* 向下节点 */
  }* level;                     /* 列向层级数组 */
} xset_node_t;
typedef xset_node_t* xset_node;
static xset_node xset_node_create(void* data, int level);
static err_t xset_node_destory(xset_node node,
                               xset_destroy_func destory_nullable);

struct xset_private {
  xset_node header;
  int level;
  int size;
  xset_cmp_func cmp;
  xset_destroy_func destroy;
};

static int xset_get_length_unsafe(xset self);
static err_t xset_clear_unsafe(xset self);
static void* xset_pop_back_unsafe(xset self);
static void* xset_pop_front_unsafe(xset self);
static void* xset_lookup_unsafe(xset self, void* data);
static err_t xset_remove_unsafe(xset self, void* data);
static err_t xset_remove_with_destory_unsafe(
    xset self, void* data, xset_destroy_func destory_nullable);
static err_t xset_insert_unsafe(xset self, void* data);

inline static int xset_random_level(void) {
  int level = 1;
  while ((rand() / (double)RAND_MAX) < XSET_PROBABILITY &&
         level < XSET_MAX_DEPTH) {
    level++;
  }
  return level;
}
inline static int xset_level(xset self) { return self->level; }

xset xset_create(xset_cmp_func cmp, xset_destroy_func destroy) {
  if (cmp == NULL || destroy == NULL) return NULL;

  xset self = xbox_malloc(sizeof(struct xset_private));
  if (self == NULL) return NULL;

  self->level = 1;
  self->size = 0;

  self->header = xset_node_create(NULL, XSET_MAX_DEPTH);
  if (self->header == NULL) {
    xbox_free(self);
    return NULL;
  }

  self->cmp = cmp;
  self->destroy = destroy;

  return self;
}

struct xset_duplicate_context {
  xset new_set;
  xset_clone_data_func clone;
};
static void duplicate_data(const void* data, void* closure) {
  struct xset_duplicate_context* ctx = closure;

  xset_insert(ctx->new_set, ctx->clone(data));
}
xset xset_duplicate(const xset self, xset_clone_data_func clone) {
  if (self == NULL || clone == NULL) return NULL;

  xset new_set = xset_create(self->cmp, self->destroy);
  if (new_set == NULL) return NULL;

  struct xset_duplicate_context ctx = {new_set, clone};

  xset_list_data(self, duplicate_data, &ctx);

  return new_set;
}

void* xset_pop_front(xset self) { return xset_pop_front_unsafe(self); }

void* xset_pop_back(xset self) { return xset_pop_back_unsafe(self); }

err_t xset_insert(xset self, void* data) {
  return xset_insert_unsafe(self, data);
}

err_t xset_destroy(xset self) {
  if (self == NULL) return X_RET_INVAL;

  xset_clear(self);

  if (self->header) {
    xset_node_destory(self->header, self->destroy);
  }

  xbox_free(self);
  return X_RET_OK;
}

err_t xset_remove(xset self, void* data) {
  return xset_remove_unsafe(self, data);
}

void* xset_lookup(xset self, void* data) {
  return xset_lookup_unsafe(self, data);
}

err_t xset_list_lookup(xset self,
                       const void* data,
                       void** matchs,
                       size_t matchs_size,
                       xset_cmp_func match_func) {
  if (self == NULL || data == NULL || matchs == NULL || matchs_size == 0 ||
      match_func == NULL)
    return X_RET_INVAL;

  err_t match_num = 0;
  xset_node x = self->header->level[0].forward;

  while (x) {
    if (match_func(x->data, data) == 0) {
      matchs[match_num++] = x->data;
      if (match_num == matchs_size) break;
    }

    x = x->level[0].forward;
  }

  if (match_num == 0) return X_RET_NOTENT;

  return match_num;
}

int xset_length(xset self) { return xset_get_length_unsafe(self); }

err_t xset_clear(xset self) {
  if (self == NULL) return X_RET_INVAL;

  if (xset_length(self) == 0) return X_RET_OK;

  return xset_clear_unsafe(self);
}

err_t xset_list_data(xset self, xset_visitor_func visitor, void* closure) {
  if (self == NULL || visitor == NULL) return X_RET_INVAL;

  xset_node x = self->header->level[0].forward;

  while (x) {
    visitor(x->data, closure);

    x = x->level[0].forward;
  }

  return X_RET_OK;
}

static xset_node xset_node_create(void* data, int level) {
  xset_node node = (xset_node)xbox_malloc(sizeof(xset_node_t));
  node->data = data;
  node->level = (struct xset_node_level*)
      xbox_calloc(level, sizeof(struct xset_node_level));
  node->backward = NULL;

  for (int i = 0; i < level; ++i) {
    node->level[i].forward = NULL;
  }

  return node;
}

static err_t xset_node_destory(xset_node node,
                               xset_destroy_func destory_nullable) {
  if (node == NULL) return X_RET_INVAL;

  if (node->level) {
    if (destory_nullable && node->data) destory_nullable(node->data);

    xbox_free(node->level);
    node->level = NULL;
  }

  xbox_free(node);
  return X_RET_OK;
}

static err_t xset_clear_unsafe(xset self) {
  if (self == NULL) return X_RET_INVAL;

  xset_node x = self->header->level[0].forward;

  while (x) {
    xset_node t = x;
    x = x->level[0].forward;

    xset_node_destory(t, self->destroy);
  }

  for (int i = 0; i < XSET_MAX_DEPTH; ++i) {
    self->header->level[i].forward = NULL;
  }

  self->size = 0;
  self->level = 1;
  return X_RET_OK;
}

static err_t xset_insert_unsafe(xset self, void* data) {
  if (self == NULL || data == NULL) return X_RET_INVAL;

  xset_node path[XSET_MAX_DEPTH] = {0};
  xset_node x = self->header;

  for (int i = self->level - 1; i >= 0; --i) {
    while (x->level[i].forward &&
           self->cmp(x->level[i].forward->data, data) < 0) {
      x = x->level[i].forward;
    }
    path[i] = x;
  }

  x = x->level[0].forward;
  if (x && self->cmp(x->data, data) == 0) {
    return X_RET_EXIST;
  }

  int level = xset_random_level();
  if (level > self->level) {
    for (int i = self->level; i < level; ++i) {
      path[i] = self->header;
    }
    self->level = level;
  }

  xset_node node = xset_node_create(data, level);
  if (node == NULL) return X_RET_NOMEM;

  node->backward = path[0];

  for (int i = 0; i < level; ++i) {
    node->level[i].forward = path[i]->level[i].forward;
    path[i]->level[i].forward = node;
  }

  self->size++;
  return X_RET_OK;
}

static err_t xset_remove_unsafe(xset self, void* data) {
  return xset_remove_with_destory_unsafe(self, data, self->destroy);
}

static err_t xset_remove_with_destory_unsafe(
    xset self, void* data, xset_destroy_func destory_nullable) {
  if (self == NULL || data == NULL) return X_RET_INVAL;

  xset_node path[XSET_MAX_DEPTH] = {0};
  xset_node x = self->header;

  for (int i = self->level - 1; i >= 0; --i) {
    while (x->level[i].forward &&
           self->cmp(x->level[i].forward->data, data) < 0) {
      x = x->level[i].forward;
    }
    path[i] = x;
  }

  x = x->level[0].forward;
  if (x == NULL || self->cmp(x->data, data) != 0) return X_RET_NOTENT;

  for (int i = 0; i < self->level; ++i) {
    if (path[i]->level[i].forward != x) break;

    if (x->level[i].forward) {
      x->level[i].forward->backward = path[i];
    }

    path[i]->level[i].forward = x->level[i].forward;
  }

  xset_node_destory(x, destory_nullable);
  self->size--;

  while (self->level > 1 &&
         self->header->level[self->level - 1].forward == NULL) {
    self->level--;
  }

  return X_RET_OK;
}

static void* xset_lookup_unsafe(xset self, void* data) {
  if (self == NULL || data == NULL) return NULL;

  xset_node x = self->header;
  for (int i = self->level - 1; i >= 0; --i) {
    xset_node forward = x->level[i].forward;
    while (forward) {
      int ret = self->cmp(forward->data, data);
      if (ret < 0) {
        x = forward;
        forward = forward->level[i].forward;
      } else if (ret == 0) {
        return forward->data;
      } else {
        break;
      }
    }
  }

  return NULL;
}

static int xset_get_length_unsafe(xset self) {
  if (self == NULL) return 0;

  return self->size;
}

static void* xset_pop_back_unsafe(xset self) {
  if (self == NULL || xset_get_length_unsafe(self) == 0) return NULL;

  void* data = NULL;
  xset_node x = self->header;
  while (x->level->forward) {
    x = x->level->forward;
  }

  data = x->data;
  xset_remove_with_destory_unsafe(self, x->data, NULL);
  return data;
}

static void* xset_pop_front_unsafe(xset self) {
  if (self == NULL || xset_get_length_unsafe(self) == 0) return NULL;

  void* data = self->header->level[0].forward->data;
  xset_remove_with_destory_unsafe(self, data, NULL);
  return data;
}
