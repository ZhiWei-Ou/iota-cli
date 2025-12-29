/**
 * @brief Unix 文件操作
 * @file os_file.h
 * @author Oswin
 * @date 2025-07-01
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_FILE_H_
#define OS_FILE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "xdef.h"

/**
 * @brief Opens a file.
 * @param path The path to the file.
 * @param mode The mode to open the file in (e.g., "r", "w", "a").
 * @return A file pointer on success, or NULL on failure.
 */
FILE* os_file_open(const char* path, const char* mode);

/**
 * @brief Closes a file.
 * @param fp A file pointer to the file to be closed.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_file_close(FILE* fp);

/**
 * @brief Checks if a file exists.
 * @param path The path to the file.
 * @return xTRUE if the file exists, xFALSE otherwise.
 */
xbool_t os_file_exist(const char* path);

/**
 * @brief Gets the size of a file.
 * @param path The path to the file.
 * @return The size of the file in bytes, or 0 on failure.
 */
size_t os_file_size(const char* path);

/**
 * @brief Reads the entire content of a file.
 * @param path The path to the file.
 * @return A pointer to a buffer containing the file content. The caller is responsible for freeing this buffer. Returns NULL on failure.
 */
uint8_t* os_file_readall(const char* path);

/**
 * @brief Reads file content into a provided buffer.
 * @param path The path to the file.
 * @param buf The buffer to read the file content into.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_file_readall_to(const char* path, uint8_t* buf, size_t len);

/**
 * @brief Writes data to a file, overwriting existing content.
 * @param path The path to the file.
 * @param buf The buffer containing the data to write.
 * @param len The number of bytes to write.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_file_write(const char* path, const uint8_t* buf, size_t len);

/**
 * @brief Appends data to a file.
 * @param path The path to the file.
 * @param buf The buffer containing the data to append.
 * @param len The number of bytes to append.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_file_write_append(const char* path, const uint8_t* buf, size_t len);

/**
 * @brief Clears the content of a file.
 * @param path The path to the file.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_file_clear(const char* path);

/**
 * @brief Renames a file.
 * @param o The old file path.
 * @param n The new file path.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_rename(const char* o, const char* n);

/**
 * @brief Deletes a file.
 * @param path The path to the file.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_remove(const char* path);

/**
 * @brief Checks if a path is a directory.
 * @param path The path to check.
 * @return xTRUE if the path is a directory, xFALSE otherwise.
 */
xbool_t the_name_is_dir(const char* path);

/**
 * @brief Gets the base name from a path.
 * @param path The file path.
 * @return A pointer to the base name within the path string.
 */
const char* os_file_basename(const char* path);

/**
 * @brief Gets the directory name from a path.
 * @param path The file path.
 * @param buf The buffer to store the directory name.
 * @param len The size of the buffer.
 * @return A pointer to the buffer containing the directory name.
 */
const char* os_file_dirname(const char* path, char* buf, size_t len);

/**
 * @brief Gets the extension name from a path.
 * @param path The file path.
 * @return A pointer to the extension name within the path string, or NULL if not found.
 */
const char* os_file_extname(const char* path);

/**
 * @brief Gets the file name without the extension.
 * @param path The file path.
 * @param buf The buffer to store the file name.
 * @param len The size of the buffer.
 * @return A pointer to the buffer containing the file name.
 */
const char* os_file_name(const char* path, char* buf, size_t len);

/**
 * @brief Replaces the extension of a file path.
 * @param path The original file path.
 * @param ext The new extension.
 * @param buf The buffer to store the new path.
 * @param len The size of the buffer.
 * @return A pointer to the buffer containing the new path.
 */
const char* os_file_replace_extname(const char* path,
                                    const char* ext,
                                    char* buf,
                                    size_t len);

#ifdef __cplusplus
}
#endif
#endif /* OS_FILE_H_ */
