/**
 * @brief FreeRTOS 文件操作
 * @file os_file.h
 * @author Oswin
 * @date 2025-07-01
 * @details
 * @attention FreeRTOS does not have an in-builtin or official file system component.
 * So this file is a simple wrapper for the standard C library functions.
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
 * @brief Open a file
 *      This function is a wrapper of fopen, and then check the input paramaters
 * @param path file path
 * @param mode file open mode
 *              "r"  : read only
 *              "r+" : read and write
 *              "w"  : write only, if the file exist, clear it first
 *              "w+" : write and read, if the file exist, clear it first
 *              "a"  : append only, if the file not exist, create it
 *              "a+" : append and read
 * @param[out] FILE  The file handle
 */
FILE* os_file_open(const char* path, const char* mode);

/**
 * @brief Close a file
 *     This function is a wrapper of fclose, and then check the file handle is
 * valid
 * @param fp file handle
 *
 * @param[out] err_t The error code
 */
err_t os_file_close(FILE* fp);

/**
 * @brief Check file existence
 * @param path file path
 * @param[out] xbool_t xTRUE if the file exist, otherwise xFALSE
 */
xbool_t os_file_exist(const char* path);

/**
 * @brief Get file size
 * @param path file path
 * @param[out] size_t The file size in bytes
 */
size_t os_file_size(const char* path);

/**
 * @brief Read all data from a file
 * @param path file path
 * @param[out] uint8_t* The file data, need to be freed by caller
 */
uint8_t* os_file_readall(const char* path);

/**
 * @brief Read data from a file
 * @param path file path
 * @param buf The buffer to store the file data
 * @param len The buffer length
 * @param[out] err_t The error code
 */
err_t os_file_readall_to(const char* path, uint8_t* buf, size_t len);

/**
 * @brief Write data to a file
 * @param path file path
 * @param buf The buffer contains the data to write
 * @param len The buffer length
 * @param[out] err_t The error code
 */
err_t os_file_write(const char* path, const uint8_t* buf, size_t len);

/**
 * @brief Append data to a file
 * @param path file path
 * @param buf The buffer contains the data to append
 * @param len The buffer length
 * @param[out] err_t The error code
 */
err_t os_file_write_append(const char* path, const uint8_t* buf, size_t len);

/**
 * @brief Clear a file content
 * @param path file path
 * @param[out] err_t The error code
 */
err_t os_file_clear(const char* path);

/**
 * @brief Rename a file
 * @param o The old file path
 * @param n The new file path
 * @param[out] err_t The error code
 */
err_t os_rename(const char* o, const char* n);

/**
 * @brief Remove a file
 * @param path file path
 */
err_t os_remove(const char* path);

xbool_t the_name_is_dir(const char* path);

const char* os_file_basename(const char* path);
const char* os_file_dirname(const char* path, char* buf, size_t len);
const char* os_file_extname(const char* path);
const char* os_file_name(const char* path, char* buf, size_t len);
const char* os_file_replace_extname(const char* path,
                                    const char* ext,
                                    char* buf,
                                    size_t len);

#ifdef __cplusplus
}
#endif
#endif /* OS_FILE_H_ */
