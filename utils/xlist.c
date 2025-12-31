/**
 * @brief 双向链表
 * @file xlist.c
 * @author Oswin
 * @date 2025-11-19
 * @details double linked list
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xlist.h"

static const int8_t dummy = 0xff;

// Forward declarations for the static helper functions.
static struct xlist_node* new_node(void* data);
static void* delete_node(struct xlist_node* node);
static struct xlist_node* merge_sort_recursive(struct xlist_node* head,
                                               int (*cmp)(const void*,
                                                          const void*));
static struct xlist_node* merge(struct xlist_node* left,
                                struct xlist_node* right,
                                int (*cmp)(const void*, const void*));

xlist xlist_create(void) {
  xlist self = xbox_malloc(sizeof(struct xlist_priv));
  if (!self) return NULL;

  xlist_init(self);

  return self;
}

err_t xlist_destroy(xlist self) {
  if (!self) return X_RET_INVAL;

  struct xlist_node* node = self->head_.next_;

  while (node != &self->head_) {
    struct xlist_node* next = node->next_;
    delete_node(node);
    node = next;
  }

  xbox_free(self);

  return X_RET_OK;
}

err_t xlist_drain(xlist self, void (*free_func)(void*)) {
  if (!self) return X_RET_INVAL;

  struct xlist_node* node = self->head_.next_;

  while (node != &self->head_) {
    struct xlist_node* next = node->next_;

    if (free_func)
      free_func(delete_node(node));
    else
      delete_node(node);

    node = next;
  }

  self->length_ = 0;
  self->head_.prev_ = &self->head_;
  self->head_.next_ = &self->head_;

  return X_RET_OK;
}

err_t xlist_init(xlist self) {
  if (!self) return X_RET_INVAL;

  self->length_ = 0;
  self->head_.data_ = (void*)&dummy;
  self->head_.prev_ = &self->head_;
  self->head_.next_ = &self->head_;

  return X_RET_OK;
}

err_t xlist_push_back(xlist self, void* data) {
  if (!self) return X_RET_INVAL;

  struct xlist_node* node = new_node(data);
  if (!node) return X_RET_NOMEM;

  self->head_.prev_->next_ = node;
  node->prev_ = self->head_.prev_;
  node->next_ = &self->head_;
  self->head_.prev_ = node;

  self->length_++;

  return X_RET_OK;
}

err_t xlist_push_front(xlist self, void* data) {
  if (!self) return X_RET_INVAL;

  struct xlist_node* node = new_node(data);
  if (!node) return X_RET_NOMEM;

  self->head_.next_->prev_ = node;
  node->prev_ = &self->head_;
  node->next_ = self->head_.next_;
  self->head_.next_ = node;

  self->length_++;

  return X_RET_OK;
}

void* xlist_pop_back(xlist self) {
  if (!self) return NULL;

  struct xlist_node* node = self->head_.prev_;
  if (node == &self->head_) return NULL;

  node->prev_->next_ = &self->head_;
  self->head_.prev_ = node->prev_;
  self->length_--;

  return delete_node(node);
}

void* xlist_pop_front(xlist self) {
  if (!self) return NULL;

  struct xlist_node* node = self->head_.next_;
  if (node == &self->head_) return NULL;

  node->next_->prev_ = &self->head_;
  self->head_.next_ = node->next_;
  self->length_--;

  return delete_node(node);
}

void* xlist_at(xlist self, size_t index) {
  if (!self || index >= self->length_) return NULL;

  size_t step = 0;
  struct xlist_node* node = self->head_.next_;

  while (node != &self->head_) {
    if (step == index) return node->data_;

    node = node->next_;
    step++;
  }

  return NULL;
}

void* xlist_remove(xlist self, const void* data) {
  if (!self) return NULL;

  struct xlist_node* node = self->head_.next_;

  while (node != &self->head_) {
    if (node->data_ == data) {
      node->prev_->next_ = node->next_;
      node->next_->prev_ = node->prev_;
      self->length_--;
      return delete_node(node);
    }
    node = node->next_;
  }

  return NULL;
}

err_t xlist_sort(xlist self, xlist_cmp_fn cmp) {
  if (!self || !cmp) {
    return X_RET_INVAL;
  }

  struct xlist_node* first_node = self->head_.next_;

  // If the list is empty or has only one element, it is already sorted.
  if (first_node == &self->head_ || first_node->next_ == &self->head_) {
    return X_RET_OK;
  }

  // The recursive merge sort works on a standard NULL-terminated list.
  // We break the circular links to the sentinel head node.
  struct xlist_node* last_node = self->head_.prev_;
  last_node->next_ = NULL;   // The last node should point to NULL.
  first_node->prev_ = NULL;  // The first node's prev should be NULL.

  // This will return the head of the new, fully sorted linear list.
  first_node = merge_sort_recursive(first_node, cmp);

  // The head sentinel's 'next' should point to the new head of the sorted list.
  self->head_.next_ = first_node;
  first_node->prev_ = &self->head_;

  // Find the new last node of the sorted list by traversing to the end.
  struct xlist_node* current = first_node;
  while (current->next_ != NULL) {
    current = current->next_;
  }
  // 'current' is now the new tail of the sorted list.

  // Link the new tail back to the head sentinel.
  current->next_ = &self->head_;
  self->head_.prev_ = current;

  return X_RET_OK;
}

size_t xlist_length(xlist self) {
  if (!self) return 0;

  return self->length_;
}

/**
 * @brief Recursively sorts a list by splitting it and merging the sorted halves.
 *
 * @param head The head of the (linear) list segment to sort.
 * @param cmp  The comparison function.
 * @return The head of the sorted list segment.
 */
static struct xlist_node* merge_sort_recursive(struct xlist_node* head,
                                               int (*cmp)(const void*,
                                                          const void*)) {
  // Base case: if the list is empty or has only one node, it's already sorted.
  if (!head || !head->next_) {
    return head;
  }

  // --- 1. Split the list into two halves using the fast/slow pointer method
  // ---
  struct xlist_node* slow = head;
  struct xlist_node* fast = head->next_;

  // Move 'fast' two steps at a time and 'slow' one step at a time.
  while (fast != NULL) {
    fast = fast->next_;
    if (fast != NULL) {
      slow = slow->next_;
      fast = fast->next_;
    }
  }
  // When 'fast' reaches the end, 'slow' is at the node just before the
  // midpoint.

  // 'right_half' starts from the node after 'slow'.
  struct xlist_node* right_half = slow->next_;
  // 'left_half' is the original 'head'.

  // Break the link between the two halves to form two separate lists.
  slow->next_ = NULL;
  right_half->prev_ = NULL;

  // --- 2. Recursively sort both halves ---
  struct xlist_node* left_sorted = merge_sort_recursive(head, cmp);
  struct xlist_node* right_sorted = merge_sort_recursive(right_half, cmp);

  // --- 3. Merge the two sorted halves back together ---
  return merge(left_sorted, right_sorted, cmp);
}

static struct xlist_node* new_node(void* data) {
  struct xlist_node* node = xbox_malloc(sizeof(struct xlist_node));
  if (node) {
    node->data_ = data;
    node->prev_ = NULL;
    node->next_ = NULL;
  }
  return node;
}

static void* delete_node(struct xlist_node* node) {
  void* ret = node->data_;
  xbox_free(node);
  return ret;
}

/**
 * @brief Merges two sorted doubly-linked lists into a single sorted list.
 *
 * @param left  The head of the first sorted list.
 * @param right The head of the second sorted list.
 * @param cmp   The comparison function.
 * @return The head of the final merged, sorted list.
 */
static struct xlist_node* merge(struct xlist_node* left,
                                struct xlist_node* right,
                                int (*cmp)(const void*, const void*)) {
  // If one list is empty, the other is the result.
  if (!left) return right;
  if (!right) return left;

  struct xlist_node* result_head;

  // --- 1. Pick the initial head for the merged list ---
  // The smaller of the two heads becomes the new head.
  if (cmp(left->data_, right->data_) <= 0) {
    result_head = left;
    left = left->next_;
  } else {
    result_head = right;
    right = right->next_;
  }
  result_head->prev_ = NULL;  // The new head has no previous node.

  struct xlist_node* current_tail = result_head;

  // --- 2. Iterate through both lists, appending the smaller node each time ---
  while (left != NULL && right != NULL) {
    if (cmp(left->data_, right->data_) <= 0) {
      current_tail->next_ = left;
      left->prev_ = current_tail;
      current_tail = left;
      left = left->next_;
    } else {
      current_tail->next_ = right;
      right->prev_ = current_tail;
      current_tail = right;
      right = right->next_;
    }
  }

  // --- 3. Append the remaining nodes from whichever list is not yet empty ---
  if (left != NULL) {
    current_tail->next_ = left;
    left->prev_ = current_tail;
  } else {  // if (right != NULL)
    current_tail->next_ = right;
    right->prev_ = current_tail;
  }

  return result_head;
}
