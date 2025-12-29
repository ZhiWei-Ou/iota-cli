/**
 * @brief RT-Thread 文件操作
 * @file os_file.c
 * @author Oswin
 * @date 2025-07-01
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_file.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

  struct stat buf;
  return stat(path, &buf) == 0;
}

size_t os_file_size(const char* path) {
  if (path == NULL) return 0;

  struct stat buf;
  stat(path, &buf);
  return buf.st_size;
}

uint8_t* os_file_readall(const char* path) {
  if (path == NULL) return NULL;

  size_t len = os_file_size(path);
  /* reserve one byte for '\0' */
  uint8_t* buf = calloc(len + 1, sizeof(uint8_t));
  os_file_readall_to(path, buf, len);
  return buf;
}

err_t os_file_readall_to(const char* path, uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL || len <= 0) return X_RET_INVAL;

  FILE* fp = os_file_open(path, "rb");
  if (fp == NULL) return X_RET_ERROR;

  size_t n = fread(buf, 1, len, fp);
  fclose(fp);

  return n <= len ? X_RET_OK : X_RET_ERROR;
}

err_t os_file_write(const char* path, const uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return X_RET_INVAL;

  FILE* fp = os_file_open(path, "wb");
  if (fp == NULL) return X_RET_ERROR;

  fwrite(buf, 1, len, fp);
  fclose(fp);

  return X_RET_OK;
}

err_t os_file_write_append(const char* path, const uint8_t* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return X_RET_INVAL;

  FILE* fp = NULL;

  if (os_file_exist(path)) {
    fp = os_file_open(path, "ab");
  } else {
    return os_file_write(path, buf, len);
  }

  fwrite(buf, 1, len, fp);
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

const char* os_file_basename(const char* path) {
  if (path == NULL) return "";

  const char* p = strrchr(path, '/');
  return p == NULL ? path : p + 1;
}

xbool_t the_name_is_dir(const char* path) {
  if (path == NULL) return xFALSE;

  return path[strlen(path) - 1] == '/';
}

const char* os_file_dirname(const char* path, char* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);
  const char* p = strrchr(path, '/');
  if (p == NULL) return "";

  strncpy(buf, path, p - path);
  buf[p - path] = '\0';
  return buf;
}

const char* os_file_extname(const char* path) {
  if (path == NULL) return "";

  const char* p = strrchr(path, '/');
  if (p != NULL) path = p + 1;

  p = strrchr(path, '.');
  return p == NULL ? "" : p;
}

const char* os_file_name(const char* path, char* buf, size_t len) {
  if (path == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);
  const char* p = strrchr(path, '/');
  if (p == NULL) return "";

  strncpy(buf, p + 1, len);
  buf[len - 1] = '\0';
  return buf;
}

const char* os_file_replace_extname(const char* path,
                                    const char* ext,
                                    char* buf,
                                    size_t len) {
  if (path == NULL || ext == NULL || buf == NULL || len == 0) return "";

  memset(buf, 0, len);

  if (the_name_is_dir(path)) return "";

  const char* last_slash = strrchr(path, '/');

  const char* dot = strrchr(path, '.');

  const char* end_of_basename = path + strlen(path);

  if (dot != NULL && (last_slash == NULL || dot > last_slash)) {
    end_of_basename = dot;
  }

  size_t basename_len = end_of_basename - path;

  const char* new_ext = (ext[0] == '.') ? ext : ".";
  if (ext[0] != '.') {
    snprintf(buf, len, "%.*s%s%s", (int)basename_len, path, new_ext, ext);
  } else {
    snprintf(buf, len, "%.*s%s", (int)basename_len, path, new_ext);
  }

  buf[len - 1] = '\0';

  return buf;
}
