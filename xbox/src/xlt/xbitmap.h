/**
 * @brief 位图
 * @file xbitmap.h
 * @author Oswin
 * @date 2025-11-25
 * @details Provides functions to create and manipulate bitmaps.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XBITMAP__H_
#define XTOOL_XBITMAP__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"

/** @brief The number of bits in one block of the bitmap (typically a byte). */
#define BITMAP_BLOCK_SIZE 8  // 1byte = 8bit

/** @brief Opaque handle to a bitmap object. */
typedef struct xbitmap_priv* xbitmap;

/**
 * @brief Creates a new bitmap object.
 *
 * This function allocates and initializes a new bitmap capable of holding
 * `size` * 8 bits.
 *
 * @param size The number of bytes for the bitmap's underlying data storage. Each byte holds 8 bits.
 * @return A pointer to the newly created bitmap object, or NULL if memory allocation fails or size is 0.
 */
xbitmap xbitmap_create(size_t size);

/**
 * @brief Deletes a bitmap object and frees its associated memory.
 *
 * @param b A pointer to the bitmap object to be deleted.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL.
 */
err_t xbitmap_destroy(xbitmap b);

/**
 * @brief Returns the total number of bits the bitmap can hold.
 *
 * @param b A pointer to the bitmap object.
 * @return The total number of bits in the bitmap, or 0 if the bitmap pointer is NULL.
 */
size_t xbitmap_size(xbitmap b);

/**
 * @brief Sets a specific bit at the given index in the bitmap.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to set.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL, its bits are NULL, or the index is out of bounds.
 */
err_t xbitmap_set(xbitmap b, size_t index);

/**
 * @brief Clears a specific bit at the given index in the bitmap.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to clear.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL, its bits are NULL, or the index is out of bounds.
 */
err_t xbitmap_clear(xbitmap b, size_t index);

/**
 * @brief Checks if a specific bit at the given index in the bitmap is set.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to check.
 * @return xTRUE if the bit is set, xFALSE if the bit is clear or if the arguments are invalid.
 */
xbool_t xbitmap_is_set(xbitmap b, size_t index);

/**
 * @brief Displays the bitmap in a human-readable format.
 *
 * Each row shows 'row_bits' number of bits. Set bits are marked with '*', clear
 * bits with ' '.
 *
 * @param b A pointer to the bitmap object.
 * @param row_bits The number of bits to display per row.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL.
 */
err_t xbitmap_display(xbitmap b, size_t row_bits);

/**
 * @brief Checks if any bit is set within a 32-bit block of the bitmap.
 *
 * This function treats the bitmap as a series of 32-bit blocks and checks if
 * any bit within the block starting at `start` * 32 is set.
 *
 * @param b A pointer to the bitmap object.
 * @param start The starting block index (each block is 32 bits).
 * @return xTRUE if any bit in the specified 32-bit block is set, xFALSE otherwise or if arguments are invalid.
 */
xbool_t xbitmap_32bits_block_has_set(xbitmap b, size_t start);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XTOOL_XBITMAP__H_ */
