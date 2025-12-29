/**
 * @brief 位图
 * @file xbitmap.c
 * @author Oswin
 * @date 2025-11-25
 * @details Provides functions to create and manipulate bitmaps.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xbitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xbitmap_priv {
  size_t size;   /* the total number of bits */
  uint8_t* bits; /* the bits array */
};

/**
 * @brief Creates a new bitmap object.
 *
 * This function allocates and initializes a new bitmap capable of holding
 * `size` * 8 bits. It allocates memory for the `xbitmap_priv` structure and the
 * underlying `uint8_t` array for the bits.
 *
 * @param size The number of bytes for the bitmap's underlying data storage. Each byte holds 8 bits.
 * @return A pointer to the newly created bitmap object, or NULL if memory allocation fails or size is 0.
 */
xbitmap xbitmap_create(size_t size) {
  if (0 >= size) {
    return NULL;
  }

  // Allocate memory for the bitmap structure
  xbitmap b = xbox_calloc(1, sizeof(struct xbitmap_priv));
  if (!b) {
    return NULL;
  }
  // Allocate memory for the actual bits
  void* buf = xbox_calloc(size, sizeof(uint8_t));
  if (!buf) {
    xbox_free(b);  // Free the previously allocated bitmap structure
    return NULL;
  }
  b->bits = buf;
  b->size = BITMAP_BLOCK_SIZE * size;  // Total number of bits
  return b;
}

/**
 * @brief Returns the total number of bits the bitmap can hold.
 *
 * @param b A pointer to the bitmap object.
 * @return The total number of bits in the bitmap, or 0 if the bitmap pointer is NULL.
 */
size_t xbitmap_size(xbitmap b) {
  if (!b) {
    return 0;
  }

  return b->size;
}

/**
 * @brief Deletes a bitmap object and frees its associated memory.
 *
 * This function frees the memory allocated for the bits array and then the
 * bitmap structure itself.
 *
 * @param b A pointer to the bitmap object to be deleted.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL.
 */
err_t xbitmap_destroy(xbitmap b) {
  if (!b) {
    return X_RET_INVAL;
  }

  if (b->bits) {
    xbox_free(b->bits);
  }

  xbox_free(b);
  return X_RET_OK;
}

/**
 * @brief Internal helper function to validate bitmap arguments and index.
 *
 * Checks if the bitmap object and its bit array are not NULL, and if the index
 * is within valid bounds.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index to validate.
 * @return xTRUE if the arguments are valid, xFALSE otherwise.
 */
static xbool_t bitmap_arg_is_valid(xbitmap b, size_t index) {
  if (!b || !b->bits || b->size <= index) {
    return xFALSE;
  } else {
    return xTRUE;
  }
}

/**
 * @brief Sets a specific bit at the given index in the bitmap.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to set.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL, its bits are NULL, or the index is out of bounds.
 */
err_t xbitmap_set(xbitmap b, size_t index) {
  if (!bitmap_arg_is_valid(b, index)) {
    return X_RET_INVAL;
  }

  size_t byte = index / BITMAP_BLOCK_SIZE;
  uint8_t offset = index % BITMAP_BLOCK_SIZE;
  b->bits[byte] |= (0x01 << offset);
  return X_RET_OK;
}

/**
 * @brief Clears a specific bit at the given index in the bitmap.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to clear.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL, its bits are NULL, or the index is out of bounds.
 */
err_t xbitmap_clear(xbitmap b, size_t index) {
  if (!bitmap_arg_is_valid(b, index)) {
    return X_RET_INVAL;
  }

  size_t byte = index / BITMAP_BLOCK_SIZE;
  uint8_t offset = index % BITMAP_BLOCK_SIZE;
  b->bits[byte] &= (~(0x01 << offset));
  return X_RET_OK;
}

/**
 * @brief Checks if a specific bit at the given index in the bitmap is set.
 *
 * @param b A pointer to the bitmap object.
 * @param index The 0-based index of the bit to check.
 * @return xTRUE if the bit is set, xFALSE if the bit is clear or if the arguments are invalid.
 */
xbool_t xbitmap_is_set(xbitmap b, size_t index) {
  if (!bitmap_arg_is_valid(b, index)) {
    return xFALSE;
  }

  size_t byte = index / BITMAP_BLOCK_SIZE;
  uint8_t offset = index % BITMAP_BLOCK_SIZE;
  uint8_t shift = (0x01 << offset) & b->bits[byte];
  return (shift != 0);
}

/**
 * @brief Displays the bitmap in a human-readable format to standard output.
 *
 * Each row shows 'row_bits' number of bits. Set bits are marked with '*', clear
 * bits with ' '.
 *
 * @graph
 * #0 |*| | | | | | | |
 * #1 | | |*| | | | | |
 * #2 | | | |*| | | | |
 * #3 | | | | | |*| | |
 * @endgraph
 *
 * @param b A pointer to the bitmap object.
 * @param row_bits The number of bits to display per row.
 * @return X_RET_OK  on success, or X_RET_INVAL if the bitmap pointer is NULL.
 */
err_t xbitmap_display(xbitmap b, size_t row_bits) {
  if (b == NULL) {
    return X_RET_INVAL;
  }

  size_t size = xbitmap_size(b);
  size_t row = size / row_bits;
  for (size_t i = 0; i < row; i++) {
    printf("#%d |", (int)i);
    for (size_t j = 0; j < row_bits; j++) {
      char mark = ' ';
      if (xbitmap_is_set(b, i * row_bits + j)) {
        mark = '*';
      }
      printf("%c|", mark);
    }
    printf("\n");
  }

  // Print the remaining bits in the last row
  if (size % row_bits != 0) {
    printf("#%d |", (int)row);
    for (size_t i = row * row_bits; i < size; i++) {
      char mark = ' ';
      if (xbitmap_is_set(b, i)) {
        mark = '*';
      }
      printf("%c|", mark);
    }
    printf("\n");
  }
  return X_RET_OK;
}

/**
 * @brief Checks if any bit is set within a 32-bit block of the bitmap.
 *
 * This function treats the bitmap as a series of 32-bit blocks and checks if
 * any bit within the block starting at `start` * 32 is set. It does this by
 * reading a 32-bit value directly from the corresponding memory location and
 * checking if it's non-zero.
 *
 * @param b A pointer to the bitmap object.
 * @param start The starting block index (each block is 32 bits).
 * @return xTRUE if any bit in the specified 32-bit block is set, xFALSE otherwise or if arguments are invalid.
 */
xbool_t xbitmap_32bits_block_has_set(xbitmap b, size_t start) {
  uint32_t value = 0;  // Changed to uint32_t to match block_bits
  size_t block_bits = 32;
  // Calculate the end index for validation. The last valid index is (start *
  // block_bits + block_bits - 1). This needs to be checked against the total
  // size of the bitmap.
  size_t end_bit_index = (start + 1) * block_bits - 1;

  // Validate the bitmap and the calculated end index.
  // Note: bitmap_arg_is_valid checks if 'index' is within [0, b->size - 1].
  // So, we need to check the last bit of the block.
  if (!bitmap_arg_is_valid(b, end_bit_index)) {
    // If the end_bit_index is out of bounds, it's an invalid argument for this
    // function unless the entire block is beyond the bitmap's capacity.
    // However, if the start of the block is valid but the end is not, we still
    // return X_RET_INVAL as we cannot check a full 32-bit block.
    return xFALSE;  // Or consider a different error code like X_OutOfBounds if
                    // appropriate
  }

  // Calculate the byte offset for the start of the 32-bit block.
  // (start * block_bits) gives the starting bit index of the block.
  // Dividing by BITMAP_BLOCK_SIZE (8) converts it to a byte index.
  void* addr = &b->bits[start * block_bits / BITMAP_BLOCK_SIZE];

  // Copy 4 bytes (32 bits) from the bitmap's bit array into 'value'.
  // This assumes little-endian or that we don't care about the order of bits
  // within the 32-bit block, only if any are set.
  memcpy(&value, addr, sizeof(value));

  // If 'value' is non-zero, it means at least one bit in the 32-bit block is
  // set.
  return value != 0 ? xTRUE : xFALSE;
}
