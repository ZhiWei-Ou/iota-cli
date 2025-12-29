/**
 * @brief RT-Thread 文件操作
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

FILE* os_file_open(const char* path, const char* mode);
err_t os_file_close(FILE* fp);
xbool_t os_file_exist(const char* path);
size_t os_file_size(const char* path);
uint8_t* os_file_readall(const char* path);
err_t os_file_readall_to(const char* path, uint8_t* buf, size_t len);
err_t os_file_write(const char* path, const uint8_t* buf, size_t len);
err_t os_file_write_append(const char* path, const uint8_t* buf, size_t len);
err_t os_file_clear(const char* path);

err_t os_rename(const char* o, const char* n);
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
