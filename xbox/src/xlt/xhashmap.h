/**
 * @brief xhashmap is an @ref xhash that supports automatic expansion
 * @file xhashmap.h
 * @author Oswin
 * @date 2025-12-10
 * @details xhashmap automatically expands its capacity when the load factor reaches 66% to maintain efficient insert operations
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XLT_HASHMAP_H_
#define XLT_HASHMAP_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

/** @brief The default initial capacity for a new xhashmap. */
#define X_HASHMAP_DEFAULT_CAPACITY (16)

/** @brief Opaque handle to an xhashmap instance. */
typedef struct xhashmap_priv* xhashmap;

/** @brief Function pointer for destroying element data. */
typedef void (*xhashmap_destroy_fn)(void* data);

/**
 * @brief Creates a new xhashmap instance with default capacity and no destroy function.
 * @return A handle to the new xhashmap instance. (NULL on failure. @ref xhashmap)
 */
#define xhashmap_create() xhashmap_create_ex(NULL, X_HASHMAP_DEFAULT_CAPACITY)

/**
 * @brief Creates a new xhashmap instance with specified destroy function and initial capacity.
 * @param destroy_nullable Optional function pointer for destroying element data. Can be NULL.
 * @param initial_capacity The initial capacity of the hashmap. Must be a power of 2.
 * @return A handle to the new xhashmap instance. (NULL on failure. @ref xhashmap)
 */
xhashmap xhashmap_create_ex(xhashmap_destroy_fn destroy_nullable,
                            size_t initial_capacity);

/**
 * @brief Gets the number of elements currently in the hashmap.
 * @param map The xhashmap instance.
 * @return The number of elements in the hashmap.
 */
size_t xhashmap_length(xhashmap map);

/**
 * @brief Gets the total capacity of the hashmap.
 * @param map The xhashmap instance.
 * @return The capacity of the hashmap.
 */
size_t xhashmap_capacity(xhashmap map);

/**
 * @brief Inserts a new key-data pair into the hashmap.
 * @param map The xhashmap instance.
 * @param key The key string. Must be a null-terminated string.
 * @param data Pointer to the data to associate with the key. The hashmap takes ownership of this pointer.
 * @return X_RET_OK  on success.
 * @return X_RET_EXIST if the key already exists in the hashmap.
 */
err_t xhashmap_insert(xhashmap map, const char* key, void* data);

/**
 * @brief Looks up the data associated with a given key in the hashmap.
 * @param map The xhashmap instance.
 * @param key The key string to look up.
 * @return Pointer to the associated data if found, or NULL if the key does not exist.
 */
void* xhashmap_lookup(xhashmap map, const char* key);

/**
 * @brief Looks up the data associated with a given key in the hashmap using a custom comparison function.
 * @param map The xhashmap instance.
 * @param key The key to look up.
 * @param out_data Pointer to store the found data.
 * @param out_size Size of the output data buffer.
 * @param cmp_fn Custom comparison function to compare keys. Should return 0 if equal.
 * @return The number of matched items found.
 */
int xhashmap_lookup_ex(xhashmap map,
                       const void* key,
                       void** out_data,
                       size_t out_size,
                       int (*cmp_fn)(const void* item, const void* key));

/**
 * @brief Removes a key-data pair from the hashmap and destroys the data.
 * @param map The xhashmap instance.
 * @param key The key string to remove.
 * @return X_RET_OK  on success.
 * @return X_RET_NOTENT if the key does not exist in the hashmap.
 */
err_t xhashmap_remove(xhashmap map, const char* key);

/**
 * @brief Removes a key-data pair from the hashmap and returns the data without destroying it.
 * @param map The xhashmap instance.
 * @param key The key string to remove.
 * @return Pointer to the associated data if found, or NULL if the key does not exist. The caller takes ownership of the returned pointer.
 */
void* xhashmap_remove_hold(xhashmap map, const char* key);

/**
 * @brief Clears all elements from the hashmap, destroying their data.
 * @param map The xhashmap instance.
 * @return X_RET_OK  on success.
 */
err_t xhashmap_clear(xhashmap map);

/**
 * @brief Destroys the hashmap and frees all associated memory.
 * @param map The xhashmap instance.
 * @return X_RET_OK  on success.
 */
err_t xhashmap_destroy(xhashmap map);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XLT_HASHMAP_H_ */
