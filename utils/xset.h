/**
 * @brief 跳表算法容器
 * @file xset.h
 * @author Oswin
 * @date 2025-07-04
 * @details An implementation of an ordered set based on a skip list.
 *           It provides efficient insertion, removal, and lookup of elements
 *           while maintaining sorted order.
 * @see https://en.wikipedia.org/wiki/Skip_list
 * @see test/unix/xset_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XSET__H_
#define XTOOL_XSET__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

/**
 * @brief Opaque handle to an ordered set instance.
 */
typedef struct xset_private* xset;

/**
 * @brief Function pointer for comparing two elements in the set.
 * @param a Pointer to the first element.
 * @param b Pointer to the second element.
 * @return  < 0 if a < b
 *          = 0 if a == b
 *          > 0 if a > b
 */
typedef int (*xset_cmp_func)(const void* a, const void* b);

/**
 * @brief Function pointer for destroying/freeing an element's data when it is removed from the set.
 * @param data Pointer to the element's data to be freed.
 */
typedef void (*xset_destroy_func)(void* data);

/**
 * @brief Function pointer for cloning an element's data. Used for duplicating the set.
 * @param data Pointer to the element's data to be cloned.
 * @return A pointer to the newly allocated and copied data.
 */
typedef void* (*xset_clone_data_func)(const void* data);

/**
 * @brief Function pointer for visiting an element during iteration.
 * @param data Pointer to the element's data.
 * @param closure A user-provided pointer passed through to the visitor function.
 */
typedef void (*xset_visitor_func)(const void* data, void* closure);

/**
 * @brief Creates a new, empty ordered set.
 * @param cmp The comparison function used to order elements. Must not be NULL.
 * @param destroy The function used to free element data. Can be NULL if data should not be freed.
 * @return A handle to the new set, or NULL on failure.
 */
xset xset_create(xset_cmp_func cmp, xset_destroy_func destroy);

/**
 * @brief Creates a deep copy of an existing set.
 * @param self The set to duplicate.
 * @param clone The function used to clone each element's data. If NULL, the data pointers are copied directly (shallow copy).
 * @return A handle to the new duplicated set, or NULL on failure.
 */
xset xset_duplicate(const xset self, xset_clone_data_func clone);

/**
 * @brief Destroys a set and frees all associated memory, including the elements themselves using the destroy function.
 * @param self The set to destroy.
 * @return X_RET_OK  on success.
 */
err_t xset_destroy(xset self);

/**
 * @brief Removes and returns the first element in the set (the smallest).
 * @param self The set.
 * @return A pointer to the data of the removed element, or NULL if the set is empty. The caller is responsible for the data.
 */
void* xset_pop_front(xset self);

/**
 * @brief Removes and returns the last element in the set (the largest).
 * @param self The set.
 * @return A pointer to the data of the removed element, or NULL if the set is empty. The caller is responsible for the data.
 */
void* xset_pop_back(xset self);

/**
 * @brief Inserts a new element into the set while maintaining order.
 *        If an element with the same value (according to the compare function)
 * already exists, the insertion will fail.
 * @param self The set.
 * @param data The data to insert. The set takes ownership of this pointer.
 * @return X_RET_OK  on success, X_RET_EXIST if the element is already in the set, or another error code on failure.
 */
err_t xset_insert(xset self, void* data);

/**
 * @brief Removes an element from the set that matches the given data.
 * @param self The set.
 * @param data The data to find and remove. A temporary object can be used for lookup as long as the compare function can handle it.
 * @return X_RET_OK  on success, X_RET_NOTENT if the element is not in the set.
 */
err_t xset_remove(xset self, void* data);

/**
 * @brief Searches for an element in the set.
 * @param self The set.
 * @param data The data to search for. A temporary object can be used for lookup.
 * @return A pointer to the data of the found element, or NULL if not found. This pointer is still owned by the set.
 */
void* xset_lookup(xset self, void* data);

/**
 * @brief Searches for all elements matching a custom criterion and returns them.
 * @param self The set.
 * @param data A template object to match against.
 * @param[out] matchs A user-provided array to store pointers to matching elements.
 * @param matchs_size The maximum number of matches the `matchs` array can hold.
 * @param match_func A custom comparison function for matching. If NULL, the set's default comparator is used.
 * @return The number of matching elements found and stored in `matchs`.
 */
int xset_list_lookup(xset self,
                     const void* data,
                     void** matchs,
                     size_t matchs_size,
                     xset_cmp_func match_func);

/**
 * @brief Gets the number of elements in the set.
 * @param self The set.
 * @return The number of elements.
 */
int xset_length(xset self);

/**
 * @brief Removes all elements from the set.
 *        The destroy function will be called for each element.
 * @param self The set.
 * @return X_RET_OK  on success.
 */
err_t xset_clear(xset self);

/**
 * @brief Iterates over all elements in the set in sorted order and calls a visitor function for each.
 * @param self The set.
 * @param visitor The function to call for each element.
 * @param closure A user-provided pointer that will be passed to the visitor function on each call.
 * @return X_RET_OK  on success.
 */
err_t xset_list_data(xset self, xset_visitor_func visitor, void* closure);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XTOOL_XSET__H_ */
