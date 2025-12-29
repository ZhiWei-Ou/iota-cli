/**
 * @brief 通用哈希容器
 * @file xhash.h
 * @author Oswin
 * @date 2025-06-26
 * @details A general-purpose hash table implementation using open addressing
 *           with double hashing for collision resolution.
 * @note This implementation requires the table size to be a power of 2 for
 *       the hashing algorithms to work correctly and efficiently.
 * @see test/unix/xhash_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef __XTOOL_HASH__H_
#define __XTOOL_HASH__H_

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a hash table instance.
 */
typedef struct xhash_priv* xhash;

/**
 * @brief Generic pointer to a hash table entry.
 */
typedef void* xhash_table;

/**
 * @brief Function pointer for a hash function.
 * @param key A pointer to the key to be hashed.
 * @return The 32-bit hash value.
 */
typedef uint32_t (*xhash_func)(const void* key);

/**
 * @brief Function pointer for comparing a stored element with a key.
 * @param elem A pointer to the element stored in the hash table.
 * @param key A pointer to the key to compare against.
 * @return 0 if the element's key matches the provided key, non-zero otherwise.
 */
typedef int (*xhash_cmp_func)(const void* elem, const void* key);

/**
 * @brief Function pointer for destroying/freeing an element's data.
 * @param data Pointer to the element's data to be freed.
 */
typedef void (*xhash_destroy_func)(void* data);

/**
 * @brief Function pointer for cloning an element's data.
 * @param data Pointer to the element's data to be cloned.
 * @return A pointer to the newly allocated and copied data.
 */
typedef void* (*xhash_clone_data_func)(const void* data);

/**
 * @brief The internal structure of a hash table.
 * @details Exposed in the header to allow for static allocation via `xhash_init`.
 */
// clang-format off
struct xhash_priv {
  size_t length;      /**< Current number of elements in the table. */
  xhash_table* table; /**< The underlying array of pointers to elements. */
  size_t table_cap; /**< The total capacity of the table. Must be a power of 2. */
  xhash_func h1; /**< The primary hash function. */
  xhash_func h2; /**< The secondary hash function for double hashing probing. */
  xhash_cmp_func cmp; /**< Function to compare an element with a key. */
  xhash_destroy_func destory; /**< Function to destroy/free element data on removal. */
};
// clang-format on

/**
 * @brief Initializes a statically allocated hash table.
 * @param self A pointer to the user-managed `xhash_priv` struct.
 * @param table A user-provided buffer to be used as the hash table storage.
 * @param table_size The size of the provided `table`. Must be a power of 2.
 * @param h1 The primary hash function.
 * @param h2 The secondary hash function (for collision resolution).
 * @param cmp The key comparison function.
 * @param destory The function to free element data. Can be NULL.
 * @return X_RET_OK  on success, X_RET_INVAL if `table_size` is not a power of 2.
 */
err_t xhash_init(struct xhash_priv* self,
                 xhash_table* table,
                 size_t table_size,
                 xhash_func h1,
                 xhash_func h2,
                 xhash_cmp_func cmp,
                 xhash_destroy_func destory);

/**
 * @brief Macro to statically initialize a hash table instance.
 * @param __tbl User-provided buffer for the hash table storage.
 * @param __tbl_size Size of the provided buffer. Must be a power of 2.
 * @param __h1 Primary hash function.
 * @param __h2 Secondary hash function.
 * @param __cmp Key comparison function.
 * @param __destroy Element data destructor.
 */
#define XHASH_INITIALIZER(__tbl, __tbl_size, __h1, __h2, __cmp, __destroy) \
  {                                                                        \
      .length = 0,                                                         \
      .table = (__tbl),                                                    \
      .table_cap = (__tbl_size),                                           \
      .h1 = (__h1),                                                        \
      .h2 = (__h2),                                                        \
      .cmp = (__cmp),                                                      \
      .destory = (__destroy),                                              \
  }

/**
 * @brief Creates a new, dynamically allocated hash table.
 * @param table A user-provided buffer.
 * @param table_size The size of the buffer. Must be a power of 2.
 * @param h1 Primary hash function.
 * @param h2 Secondary hash function.
 * @param cmp Key comparison function.
 * @param destory Element data destructor.
 * @return A handle to the new hash table, or NULL on failure.
 */
xhash xhash_create(xhash_table* table,
                   size_t table_size,
                   xhash_func h1,
                   xhash_func h2,
                   xhash_cmp_func cmp,
                   xhash_destroy_func destory);

/**
 * @brief Destroys a hash table and frees all associated memory.
 * @details Calls the destroy function for each element in the table.
 * @param self The hash table to destroy.
 * @return X_RET_OK  on success.
 */
err_t xhash_destroy(xhash self);

/**
 * @brief Gets the number of elements currently in the hash table.
 * @param self The hash table.
 * @return The number of elements.
 */
size_t xhash_length(xhash self);

/**
 * @brief Gets the total capacity of the hash table.
 * @param self The hash table.
 * @return The capacity.
 */
size_t xhash_capacity(xhash self);

/**
 * @brief Inserts a new element into the hash table.
 * @param self The hash table.
 * @param data The data to insert. The table takes ownership of this pointer.
 * @return X_RET_OK  on success.
 * @return X_RET_EXIST if an element with the same key is already in the table.
 * @return X_RET_FULL if the table is full.
 * @return X_RET_ERROR if a free slot could not be found after probing (hash collision saturation).
 */
err_t xhash_insert(xhash self, void* data);

/**
 * @brief Removes and destroys an element from the hash table identified by a key.
 * @details If the element is found, the configured destroy function is called on the element's data.
 * @param self The hash table.
 * @param data A pointer to a key or a template object for comparison.
 * @return X_RET_OK  on success, X_RET_NOTENT if no matching element is found.
 */
err_t xhash_remove(xhash self, const void* data);

/**
 * @brief Removes and returns an element from the hash table identified by a key.
 * @details If the element is found, return a pointer to its data.
 * @param self The hash table.
 * @param data A pointer to a key or a template object for comparison.
 * @return A pointer to the removed element's data, or NULL if no matching element is found.
 */
void* xhash_remove_hold(xhash self, const void* data);

/**
 * @brief Removes all elements from the hash table.
 * @details Calls the destroy function for every element.
 * @param self The hash table.
 * @return X_RET_OK  on success.
 */
err_t xhash_clear(xhash self);

/**
 * @brief Looks up an element in the hash table by its key.
 * @param self The hash table.
 * @param data A pointer to a key or a template object for comparison.
 * @return A pointer to the found element's data, or NULL if not found. The pointer is still owned by the table.
 */
void* xhash_lookup(xhash self, const void* data);

/**
 * @brief Iterates over all elements in the hash table and applies a visitor function.
 * @param self The hash table.
 * @param visit The visitor function to call for each element.
 * @param closure A user-provided pointer passed through to the visitor function.
 * @return X_RET_OK  on success.
 */
err_t xhash_list_data(xhash self,
                      void (*visit)(const void* data, void* closure),
                      void* closure);

/**
 * @brief A macro for iterating over all elements in the hash table.
 * @details This provides a simple way to loop through all valid entries.
 * @code
 *  my_struct_t *item;
 *  xhash_foreach(my_hash_handle, item) {
 *      // use the `data` pointer here...
 *      printf("Item: %d\n", item->id);
 *  }
 * @endcode
 */
#define xhash_foreach(___s, ___d)                                          \
  for (size_t ___i = 0;                                                    \
       xhash_foreach_seek_and_get_nowarning(___s, &___i, (void**)&___d) == \
       X_RET_OK;                                                           \
       ++___i)

/**
 * @internal
 * @brief Move @p idx to the next valid entry in the hash table.
 *  Checks whether the index currently stored in @p idx refers to a valid slot.
 *  If it is invalid (e.g., empty or deleted), the function advances @p idx to
 *  the next valid slot. If no valid slot exists, an error code is returned.
 * @param self The hash table.
 * @param[in,out] idx  Input: current index to check.
 *                  Output: updated index of the next valid slot.@return .
 * @param[out] data Pointer to store the data of the next valid slot.
 * @return X_RET_OK on success;
 *         error code if no valid slot can be found.
 */
err_t xhash_foreach_seek_and_get_nowarning(xhash self,
                                           size_t* idx,
                                           void** data);

/**
 * @brief Searches for all elements matching a custom criterion and returns them.
 * @param self The hash table.
 * @param data A template object to match against.
 * @param[out] matches A user-provided array to store pointers to matching elements.
 * @param matches_size The maximum number of matches the `matches` array can hold.
 * @param cmp A custom comparison function for matching. If NULL, the table's default comparator is used.
 * @return The number of matching elements found (>= 0), or a negative error code on failure.
 */
int xhash_list_lookup(xhash self,
                      const void* data,
                      void** matches,
                      size_t matches_size,
                      xhash_cmp_func cmp);

/**
 * @defgroup xhash_[...] Hash Functions
 * @brief Predefined hash functions.
 * @{
 */
/**
 * @brief DJB2 hash function for a block of memory.
 */
uint32_t xhash_djb2(const void* key, size_t len);

/**
 * @brief BKDR hash function for a block of memory.
 */
uint32_t xhash_bkdr(const void* key, size_t len);

/**
 * @brief FNV-1a hash function for a block of memory.
 */
uint32_t xhash_fnv1a(const void* key, size_t len);
/** @} */

/**
 * @brief Returns the next power of two greater than or equal to @p value.
 * @param value The input value.
 * @return The next power of two greater than or equal to @p value.
 */
uint32_t xhash_next_power_of_two(uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* __XTOOL_HASH__H_ */
