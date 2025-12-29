/**
 * @brief 双向链表
 * @file xlist.h
 * @author Oswin
 * @date 2025-11-19
 * @details double linked list
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XLIST__H_
#define XTOOL_XLIST__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

/** @struct xlist_node
 *  @brief A node in a doubly linked list.
 */
struct xlist_node {
  void* data_; /**< Pointer to the data stored in this node. */
  struct xlist_node *prev_,
      *next_; /**< Pointers to the previous and next nodes in the list. */
};

/** @struct xlist_priv
 *  @brief The private representation of a doubly linked list.
 */
struct xlist_priv {
  size_t length_;          /**< Number of elements in the list. */
  struct xlist_node head_; /**< Head node (sentinel) of the list. */
};

/** @brief Opaque handle to a doubly linked list. */
typedef struct xlist_priv* xlist;

/**
 * @brief Creates and initializes a new linked list instance.
 *
 * This function allocates memory for a new list structure and initializes it
 * to a valid, empty state. The caller is responsible for freeing the memory
 * for this list by calling a corresponding destroy function (e.g.,
 * xlist_destroy) when it is no longer needed.
 *
 * @return A pointer to the newly created list (xlist), or NULL if memory
 *         allocation fails.
 */
xlist xlist_create(void);

/**
 * @brief Frees all memory used by a linked list instance.
 *
 * This function iterates through all elements in the list, freeing the memory
 * for each list node. Finally, it frees the memory for the list structure
 * `self` itself.
 *
 * @warning This function does NOT free the `data` pointers stored within each
 * node. The caller is responsible for managing the memory of the data elements
 * if they were dynamically allocated. If you need to free the data as well,
 * use `xlist_drain()` before calling this function.
 *
 * After this function is called, the `self` pointer becomes invalid and
 * must not be used again.
 *
 * @param self The list to be destroyed. Can be NULL, in which case the
 *             function does nothing.
 * @return X_RET_OK , as the operation is always considered successful.
 */
err_t xlist_destroy(xlist self);

/**
 * @brief Removes all elements from a list, optionally freeing their data.
 *
 * This function iterates through every node in the list, removing it. For each
 * node, if a non-NULL `free_func` is provided, it is called with the `data`
 * pointer from that node. This allows the caller to free the memory associated
 * with the data elements. After the optional free call, the list node itself
 * is deallocated.
 *
 * After this operation, the list `self` will be empty but will still be a
 * valid, usable list structure. It does NOT free the list `self` itself.
 *
 * @param self The list to be drained (emptied). Must not be NULL.
 * @param free_func A pointer to a function that will be called for each data
 *                  element to free its memory. If this parameter is NULL, the
 *                  data pointers are simply discarded without being freed.
 * @return X_RET_OK  on success, or X_RET_INVAL if `self` is NULL.
 *
 * @par Usage Example:
 * @code
 *   // Assuming 'my_list' contains dynamically allocated 'person_t' structs.
 *   void free_person(void *person_ptr) {
 *       free(person_ptr);
 *   }
 *
 *   // Drain the list and free each person_t struct.
 *   xlist_drain(my_list, free_person);
 *
 *   // Now the list is empty, but 'my_list' itself is still a valid pointer.
 *   xlist_destroy(my_list); // Finally, free the list structure.
 * @endcode
 */
err_t xlist_drain(xlist self, void (*free_func)(void*));

/**
 * @brief Initializes a pre-allocated linked list structure.
 *
 * This function is used to initialize a list structure that has already been
 * allocated (e.g., on the stack or as a member of another struct). It sets up
 * the internal sentinel head node so that its 'next' and 'prev' pointers
 * point to itself, representing a valid empty list.
 *
 * @param self A pointer to the list structure to be initialized.
 * @return X_RET_OK  on success, or X_RET_INVAL if `self` is NULL.
 */
err_t xlist_init(struct xlist_priv* self);

/**
 * @brief Adds a new element containing `data` to the end of the list.
 *
 * This function allocates a new node, stores the provided `data` pointer in it,
 * and appends it to the tail of the list. This is a constant-time O(1)
 * operation. The list only stores the pointer; it does not take ownership of
 * the data's memory.
 *
 * @param self The list to which the element will be added. Must not be NULL.
 * @param data A pointer to the data to be stored in the new element.
 * @return X_RET_OK  on success, X_RET_INVAL if `self` is NULL, or an error
 *         code (e.g., X_NoMemory) if node allocation fails.
 */
err_t xlist_push_back(xlist self, void* data);

/**
 * @brief Adds a new element containing `data` to the beginning of the list.
 *
 * This function allocates a new node, stores the provided `data` pointer in it,
 * and prepends it to the head of the list. This is a constant-time O(1)
 * operation. The list only stores the pointer; it does not take ownership of
 * the data's memory.
 *
 * @param self The list to which the element will be added. Must not be NULL.
 * @param data A pointer to the data to be stored in the new element.
 * @return X_RET_OK  on success, X_RET_INVAL if `self` is NULL, or an error
 *         code (e.g., X_NoMemory) if node allocation fails.
 */
err_t xlist_push_front(xlist self, void* data);

/**
 * @brief Removes and returns the last element from the list.
 *
 * This function unlinks the last node from the list, frees the memory used by
 * the node itself, and returns the `data` pointer that was stored within it.
 * This is a constant-time O(1) operation. The caller is responsible for
 * managing the memory of the data pointed to by the return value.
 *
 * @param self The list from which to pop the element. Must not be NULL.
 * @return A pointer to the data from the removed element, or NULL if the list
 *         is empty or `self` is NULL.
 */
void* xlist_pop_back(xlist self);

/**
 * @brief Removes and returns the first element from the list.
 *
 * This function unlinks the first node from the list, frees the memory used by
 * the node itself, and returns the `data` pointer that was stored within it.
 * This is a constant-time O(1) operation. The caller is responsible for
 * managing the memory of the data pointed to by the return value.
 *
 * @param self The list from which to pop the element. Must not be NULL.
 * @return A pointer to the data from the removed element, or NULL if the list
 *         is empty or `self` is NULL.
 */
void* xlist_pop_front(xlist self);

/**
 * @brief Retrieves the data pointer of the element at a specific index.
 *
 * This function traverses the list to find the element at the given zero-based
 * index. This is a linear-time O(n) operation, where 'n' is the index.
 * It does not remove the element from the list.
 *
 * @param self The list to access. Must not be NULL.
 * @param index The zero-based index of the element to retrieve.
 * @return A pointer to the data at the specified index, or NULL if `self` is
 *         NULL or the index is out of bounds.
 */
void* xlist_at(xlist self, size_t index);

/**
 * @brief Returns the number of elements currently in the list.
 *
 * This is typically a constant-time O(1) operation, assuming the list
 * structure maintains a size counter.
 *
 * @param self The list to query.
 * @return The number of elements in the list. Returns 0 if `self` is NULL.
 */
size_t xlist_length(xlist self);

/**
 * @brief Removes the first occurrence of an element with the specified data
 *        pointer from the list.
 *
 * This function searches the list for a node whose `data` field matches the
 * provided `data` pointer. If found, it unlinks the node, frees the node's
 * memory, and returns the `data` pointer. This is a linear-time O(n) operation.
 *
 * @warning This function only removes the first matching element found. If the
 *          list contains multiple elements with the same data pointer, only the
 *          first one encountered from the head of the list will be removed.
 *
 * @param self The list from which to remove the element. Must not be NULL.
 * @param data The data pointer of the element to be removed.
 * @return A pointer to the data from the removed element, or NULL if `self` is
 *         NULL or no matching element is found.
 */
void* xlist_remove(xlist self, const void* data);

/**
 * @brief Comparison function for sorting the list.
 * @param __first Pointer to the first element's data.
 * @param __second Pointer to the second element's data.
 * @return < 0 if __first < __second, 0 if __first == __second, > 0 if __first > __second.
 */
typedef int (*xlist_cmp_fn)(const void* __first, const void* __second);

/**
 * @brief Sorts the list using the provided comparison function.
 * @param self The list to sort.
 * @param cmp The comparison function.
 * @return X_RET_OK on success.
 */
err_t xlist_sort(xlist self, xlist_cmp_fn cmp);

/**
 * @brief A macro to provide a convenient and safe way to iterate over the list.
 *
 * This macro expands into a `for` loop, allowing you to process each element
 * in the list from head to tail. It is designed to be safe against the removal
 * of the current element during iteration.
 *
 * @param __l The list instance to iterate over.
 * @param __p   A variable (e.g., `void *data_ptr;`) that will hold the pointer to the
 *             data of the current element in each iteration.
 *
 * @par Usage Example:
 * @code
 *   // Assuming 'my_list' is a valid 'xlist' containing pointers to strings.
 *   const char* data;
 *   xlist_foreach(my_list, data) {
 *       printf("Element: %s\n", data);
 *   }
 * @endcode
 */
#define xlist_foreach(___l, ___p)                                      \
  for (struct xlist_node* ___n = (___l)->head_.next_, *___next = NULL; \
       ___n != &(___l)->head_                                          \
           ? (xlist_foreach_get_data_nowarning(___n, (void**)&___p),   \
              ___next = ___n->next_,                                   \
              1)                                                       \
           : 0;                                                        \
       ___n = ___next)

/**
 * @internal
 * @brief Safely assigns the data of an xlist node to a void* pointer to avoid warnings
 * @param node  Pointer to the xlist node from which to retrieve data
 * @param data  Output parameter; a pointer to void*, which will receive the value of node->data_
 */
static inline void xlist_foreach_get_data_nowarning(struct xlist_node* node,
                                                    void** data) {
  *data = node->data_;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XTOOL_XLIST__H_ */
