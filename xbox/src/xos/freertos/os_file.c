/**
 * @brief FreeRTOS 文件操作
 * @file os_file.c
 * @author Oswin
 * @date 2025-07-01
 * @details This file provides file operations for a FreeRTOS environment.
 * It uses the standard C library functions (stdio.h), assuming the underlying
 * C library and Board Support Package (BSP) provide the filesystem hooks.
 * @attention FreeRTOS does not have an in-builtin or official file system component.
 * So this file is a simple wrapper for the standard C library functions.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_file.h"

#include <stdlib.h>
#include <string.h>

FILE* os_file_open(const char* path, const char* mode) {
  if (path == NULL || mode == NULL) return NULL;

  return fopen(path, mode);
}

err_t os_file_close(FILE* fp) {
  if (fp == NULL) return X_RET_INVAL;

  return fclose(fp) == 0 ? X_RET_OK : X_RET_ERROR;
}

xbool_t os_file_exist(const char* path) {
  if (path == NULL) return xFALSE;

  FILE* fp = fopen(path, "r");
  if (fp) {
    fclose(fp);
    return xTRUE;
  }
  return xFALSE;
}

size_t os_file_size(const char* path) {
  if (path == NULL) return 0;

  FILE* fp = fopen(path, "rb");
  if (fp == NULL) return 0;

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fclose(fp);

  return (size >= 0) ? (size_t)size : 0;
}

uint8_t* os_file_readall(const char* path) {
  if (path == NULL) return NULL;

  size_t len = os_file_size(path);
  if (len == 0) {
    // If file is empty or doesn't exist, return a buffer with a null
    // terminator.
    uint8_t* empty_buf = calloc(1, sizeof(uint8_t));
    return empty_buf;
  }

  uint8_t* buf = calloc(len + 1, sizeof(uint8_t));
  if (buf == NULL) {
    return NULL;
  }

  if (os_file_readall_to(path, buf, len) != X_RET_OK) {
    free(buf);
    return NULL;
  }

  return buf;
}

err_t os_file_readall_to(const char* path, uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL) return X_RET_INVAL;

  if (len == 0) return X_RET_OK;

  FILE* fp = os_file_open(path, "rb");
  if (fp == NULL) return X_RET_ERROR;

  size_t n = fread(buf, 1, len, fp);
  fclose(fp);

  // Succeeds if at least something was read, up to len.
  // The caller of os_file_readall_to should check the file size if they need
  // exactly `len` bytes.
  return (n > 0 || len == 0) ? X_RET_OK : X_RET_ERROR;
}

err_t os_file_write(const char* path, const uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL) return X_RET_INVAL;

  FILE* fp = os_file_open(path, "wb");
  if (fp == NULL) return X_RET_ERROR;

  if (len > 0) {
    if (fwrite(buf, 1, len, fp) != len) {
      fclose(fp);
      return X_RET_ERROR;
    }
  }

  fclose(fp);
  return X_RET_OK;
}

err_t os_file_write_append(const char* path, const uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL) return X_RET_INVAL;

  FILE* fp = os_file_open(path, "ab");
  if (fp == NULL) return X_RET_ERROR;

  if (len > 0) {
    if (fwrite(buf, 1, len, fp) != len) {
      fclose(fp);
      return X_RET_ERROR;
    }
  }

  fclose(fp);
  return X_RET_OK;
}

err_t os_file_clear(const char* path) {
  if (path == NULL) return X_RET_INVAL;

  FILE* fp = os_file_open(path, "wb");
  if (fp == NULL) return X_RET_ERROR;

  fclose(fp);
  return X_RET_OK;
}

err_t os_rename(const char* o, const char* n) {
  if (o == NULL || n == NULL) return X_RET_INVAL;

  return rename(o, n) == 0 ? X_RET_OK : X_RET_ERROR;
}

err_t os_remove(const char* path) {
  if (path == NULL) return X_RET_INVAL;

  return remove(path) == 0 ? X_RET_OK : X_RET_ERROR;
}

xbool_t the_name_is_dir(const char* path) {
  (void)path;
  // Standard C I/O (stdio.h) does not provide a reliable, portable way to check
  // if a path is a directory. This functionality typically requires OS-specific
  // or filesystem-specific APIs (e.g., stat() on POSIX, f_stat() with FatFs).
  // Returning false as a safe default.
  return xFALSE;
}

const char* os_file_basename(const char* path) {
  if (path == NULL) return "";

  const char* p = strrchr(path, '/');
  return p == NULL ? path : p + 1;
}

const char* os_file_dirname(const char* path, char* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);
  const char* p = strrchr(path, '/');
  if (p == NULL) {
    // No directory part, return "." ? or empty string?
    // Let's copy unix behaviour which is empty string.
    return "";
  }
  if (p == path) {  // root like "/"
    strncpy(buf, path, 1);
    return buf;
  }

  size_t dirlen = p - path;
  if (dirlen >= len) {
    dirlen = len - 1;
  }

  strncpy(buf, path, dirlen);
  return buf;
}

const char* os_file_extname(const char* path) {
  if (path == NULL) return "";

  const char* p = strrchr(path, '.');
  const char* s = strrchr(path, '/');

  if (p == NULL || (s != NULL && p < s)) {
    return "";  // No extension or dot is in a directory part.
  }

  return p;
}

const char* os_file_name(const char* path, char* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);
  const char* basename = os_file_basename(path);
  const char* ext = os_file_extname(basename);

  size_t name_len;
  if (ext != NULL && *ext != '\0') {
    name_len = ext - basename;
  } else {
    name_len = strlen(basename);
  }

  if (name_len >= len) {
    name_len = len - 1;
  }

  strncpy(buf, basename, name_len);
  return buf;
}

const char* os_file_replace_extname(const char* path,
                                    const char* ext,
                                    char* buf,
                                    size_t len) {
  if (path == NULL || ext == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);
  const char* ext_in_path = os_file_extname(path);

  size_t base_len;
  if (ext_in_path != NULL && *ext_in_path != '\0') {
    base_len = ext_in_path - path;
  } else {
    base_len = strlen(path);
  }

  if (base_len >= len) {  // Not enough space for base
    return "";
  }

  strncpy(buf, path, base_len);

  const char* dot = (*ext == '.') ? "" : ".";
  strncat(buf, dot, len - strlen(buf) - 1);
  strncat(buf, ext, len - strlen(buf) - 1);

  return buf;
}
