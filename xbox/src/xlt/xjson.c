/**
 * @brief JSON 解析器
 * @file xjson.c
 * @author Oswin
 * @date 2025-06-26
 * @details
 * @ref https://deepwiki.com/json-c/json-c/5.1-json-pointer
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xjson.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xlog.h"

// 定义解析器的状态
typedef enum {
  STATE_NORMAL,
  STATE_IN_STRING,
  STATE_IN_SINGLE_LINE_COMMENT,
  STATE_IN_MULTI_LINE_COMMENT
} xjson_remove_comments_state_t;

/**
 * @brief 从一个类JSON字符串中移除C风格的注释 (// and / * * /).
 *
 * @param input 包含注释的输入字符串.
 * @return char* 一个新的、在堆上分配的、不含注释的字符串。
 *               调用者必须负责使用 free() 释放这块内存。
 *               如果输入为NULL或内存分配失败，则返回NULL。
 */
static char* __remove_json_comments(const char* input) {
  if (!input) {
    return NULL;
  }

  size_t input_len = strlen(input);

  // the remove comments string will not exceed the length of the original
  // string
  char* output = (char*)xbox_malloc(input_len + 1);
  if (!output) {
    perror("Failed to allocate memory");
    return NULL;
  }

  xjson_remove_comments_state_t state = STATE_NORMAL;
  xbool_t is_escaped = xFALSE;  // 用于处理字符串中的转义字符
  size_t j = 0;                 // output 字符串的索引

  for (size_t i = 0; i < input_len; ++i) {
    char current_char = input[i];

    switch (state) {
      case STATE_NORMAL:
        if (current_char == '"') {
          state = STATE_IN_STRING;
          output[j++] = current_char;
        } else if (current_char == '/' && i + 1 < input_len &&
                   input[i + 1] == '/') {
          state = STATE_IN_SINGLE_LINE_COMMENT;
          i++;  // skip the second '/' character
        } else if (current_char == '/' && i + 1 < input_len &&
                   input[i + 1] == '*') {
          state = STATE_IN_MULTI_LINE_COMMENT;
          i++;  // skip the '*'
        } else {
          output[j++] = current_char;
        }
        break;

      case STATE_IN_STRING:
        if (is_escaped) {
          // if the previous character is '\', then the current character is
          // just a normal character
          is_escaped = xFALSE;
        } else if (current_char == '\\') {
          // this is an escape character
          is_escaped = xTRUE;
        } else if (current_char == '"') {
          // string end
          state = STATE_NORMAL;
        }
        output[j++] = current_char;
        break;

      case STATE_IN_SINGLE_LINE_COMMENT:
        if (current_char == '\n') {
          state = STATE_NORMAL;
          output[j++] = current_char;
        }
        break;

      case STATE_IN_MULTI_LINE_COMMENT:
        if (current_char == '*' && i + 1 < input_len && input[i + 1] == '/') {
          state = STATE_NORMAL;
          i++;
        }
        break;
    }
  }

  output[j] = '\0';
  return output;
}

void xjson_init_hooks(malloc_fn __malloc, free_fn __free) {
  cJSON_Hooks hooks = {__malloc, __free};
  return cJSON_InitHooks(&hooks);
}

xjson_type_e xjson_type(const xjson j) {
  if (j == NULL) return XJSON_TYPE_UNKOWN;

  int t = (j->type & 0xff);

  if (t == cJSON_String) {
    return XJSON_TYPE_STRING;
  } else if (t == cJSON_Number) {
    return XJSON_TYPE_NUMBER;
  } else if (t == cJSON_NULL) {
    return XJSON_TYPE_NULL;
  } else if ((t & ((cJSON_True | cJSON_False))) != 0) {
    return XJSON_TYPE_BOOLEAN;
  } else if (t == cJSON_Array) {
    return XJSON_TYPE_ARRAY;
  } else if (t == cJSON_Object) {
    return XJSON_TYPE_OBJECT;
  } else {
    return XJSON_TYPE_UNKOWN;
  }
}

const char* xjson_type_to_string(xjson_type_e t) {
  switch (t) {
    case XJSON_TYPE_STRING:
      return "string";
    case XJSON_TYPE_NUMBER:
      return "number";
    case XJSON_TYPE_BOOLEAN:
      return "boolean";
    case XJSON_TYPE_NULL:
      return "null";
    case XJSON_TYPE_ARRAY:
      return "array";
    case XJSON_TYPE_OBJECT:
      return "object";
    case XJSON_TYPE_UNKOWN:
    default:
      return "unkown";
  }
}

xbool_t xjson_is_scalar(const xjson j) {
  if (j == NULL) return xFALSE;

  return (xjson_type(j) | XJSON_TYPE_SCALAR) == XJSON_TYPE_SCALAR;
}

xbool_t xjson_is_composite(const xjson j) {
  if (j == NULL) return xFALSE;

  return (xjson_type(j) | XJSON_TYPE_COMPOSITE) == XJSON_TYPE_COMPOSITE;
}

xbool_t xjson_is_string(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_STRING;
}

xbool_t xjson_is_number(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_NUMBER;
}

xbool_t xjson_is_boolean(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_BOOLEAN;
}

xbool_t xjson_is_null(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_NULL;
}

xbool_t xjson_is_array(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_ARRAY;
}

xbool_t xjson_is_object(const xjson j) {
  if (j == NULL) return xFALSE;

  return xjson_type(j) == XJSON_TYPE_OBJECT;
}

xjson xjson_parse(const char* string) {
  if (string == NULL) return NULL;

  return cJSON_Parse(string);
}

xjson xjson_parse_has_comments(const char* string) {
  if (string == NULL) return NULL;

  char* remove_comments = __remove_json_comments(string);
  if (remove_comments == NULL) return NULL;

  xjson j = cJSON_Parse(remove_comments);
  xbox_free(remove_comments);

  return j;
}

xjson xjson_duplicate(const xjson other) {
  if (other == NULL) return NULL;

  return cJSON_Duplicate(other, 1);
}

void xjson_delete(xjson j) {
  if (j == NULL) return;

  cJSON_Delete(j);
}

char* xjson_to_string_advance(const xjson j, xbool_t is_pretty) {
  if (j == NULL) return NULL;

  if (is_pretty)
    return cJSON_Print(j);
  else
    return cJSON_PrintUnformatted(j);
}

const char* xjson_to_string_advance_r(const xjson j,
                                      char* buffer,
                                      size_t buffer_size,
                                      xbool_t is_pretty,
                                      const char* error_retval) {
  if (j == NULL || buffer == NULL || buffer_size <= 0) {
    return error_retval;
  }

  if (cJSON_PrintPreallocated(j,
                              buffer,
                              (int)buffer_size,
                              is_pretty == xTRUE ? 1 : 0)) {
    return buffer;
  } else {
    return error_retval;
  }
}

xjson xjson_create_object(void) { return cJSON_CreateObject(); }

xjson xjson_create_array(void) { return cJSON_CreateArray(); }

xjson xjson_create_string_with_value(const char* value) {
  if (value == NULL) return cJSON_CreateString("");

  return cJSON_CreateString(value);
}

xjson xjson_create_string_empty(void) { return cJSON_CreateString(""); }

xjson xjson_create_number_with_value(double value) {
  return cJSON_CreateNumber(value);
}

xjson xjson_create_zero_number(void) { return cJSON_CreateNumber(0); }

xjson xjson_create_boolean_with_value(xbool_t value) {
  if (value == xTRUE)
    return cJSON_CreateTrue();
  else
    return cJSON_CreateFalse();
}

xjson xjson_create_false(void) { return cJSON_CreateFalse(); }

xjson xjson_create_null(void) { return cJSON_CreateNull(); }

xjson xjson_search_and_return_parent(const xjson root,
                                     const char* path,
                                     xjson* parent_nullable) {
  if (root == NULL || path == NULL) return NULL;

  /* "" means root */
  if (strcmp(path, "") == 0) return root;

  char* path_copy = xbox_strdup(path);
  if (path_copy == NULL) return NULL;

  char* saveptr = NULL;
  char* token = strtok_r(path_copy, "/", &saveptr);
  cJSON* node = root;
  cJSON* parent = NULL;

  while (token && node) {
    if (xjson_is_composite(node) == xFALSE) {
      free(path_copy);
      return NULL;
    }

    if (cJSON_IsObject(node)) {
      parent = node;
      node = cJSON_GetObjectItem(node, token);
    } else if (cJSON_IsArray(node)) {
      char* endptr;
      long idx = strtol(token, &endptr, 10);
      if (*endptr != '\0' || idx < 0) {
        free(path_copy);
        return NULL;
      }

      parent = node;
      node = cJSON_GetArrayItem(node, (int)idx);
    }

    token = strtok_r(NULL, "/", &saveptr);
  }

  free(path_copy);

  if (parent_nullable != NULL) *parent_nullable = parent;

  return node;
}

xjson xjson_search(const xjson root, const char* path) {
  return xjson_search_and_return_parent(root, path, NULL);
}

const char* xjson_dirname_r(const char* path,
                            char* buffer,
                            size_t buffer_size) {
  if (!path || !buffer || buffer_size == 0) {
    return "";
  }

  if (*path != '/') {
    return "";
  }

  const char* end = path + strlen(path);

  while (end > path && *(end - 1) == '/') {
    end--;
  }

  if (end == path) {
    strcpy(buffer, "/");
    return buffer;
  }

  const char* p = end;
  while (p > path && *(p - 1) != '/') {
    p--;
  }

  if (p == path) {
    return "";
  }

  const char* last_slash = p - 1;

  while (last_slash > path && *(last_slash - 1) == '/') {
    last_slash--;
  }

  if (last_slash == path) {
    strcpy(buffer, "/");
    return buffer;
  }

  size_t len = last_slash - path;
  if (len + 1 > buffer_size) {
    return "";
  }

  strncpy(buffer, path, len);
  buffer[len] = '\0';

  return buffer;
}
const char* xjson_basename_r(const char* path,
                             char* buffer,
                             size_t buffer_size) {
  if (!path || !buffer || buffer_size == 0) {
    return "";
  }

  if (*path != '/' || strlen(path) == 1) {
    return "";
  }

  const char* end = path + strlen(path);
  const char* start = end;

  while (start > path && *(start - 1) == '/') {
    start--;
  }

  if (start == path) {
    if (buffer_size < 2) return "";
    strcpy(buffer, "/");
    return buffer;
  }

  const char* p = start;
  while (p > path && *(p - 1) != '/') {
    p--;
  }

  size_t len = start - p;
  if (len + 1 > buffer_size) {
    return "";
  }

  strncpy(buffer, p, len);
  buffer[len] = '\0';

  return buffer;
}

char* xjson_dirname(const char* path) {
  char* d = xbox_strdup(path);

  const char* d2 = xjson_dirname_r(path, d, strlen(d) + 1);

  if (d != d2) {
    free(d);
    return NULL;
  }

  return d;
}

char* xjson_basename(const char* path) {
  char* b = xbox_strdup(path);

  const char* b2 = xjson_basename_r(path, b, strlen(b) + 1);

  if (b != b2) {
    free(b);
    return NULL;
  }

  return b;
}

xjson xjson_make_parent(const xjson root, const char* path) {
  if (root == NULL || path == NULL) return NULL;

  if (strlen(path) <= 1 || path[0] != '/') return root;

  char* path_copy = xbox_strdup(path);
  if (path_copy == NULL) return NULL;

  xjson node = root;

  char* saveptr = NULL;
  char* token = strtok_r(path_copy, "/", &saveptr);

  while (token) {
    if (cJSON_IsObject(node)) {
      xjson n = cJSON_GetObjectItem(node, token);
      if (n == NULL) {
        cJSON_AddItemToObject(node, token, n = cJSON_CreateObject());
      } else if (cJSON_IsNull(n)) {
        cJSON_ReplaceItemInObject(node, token, n = cJSON_CreateObject());
      }
      node = n;
    } else if (cJSON_IsArray(node)) {
      char* endptr;
      long idx = strtol(token, &endptr, 10);
      if (*endptr != '\0' || idx < 0) {
        free(path_copy);
        return NULL;
      }

      xjson n = cJSON_GetArrayItem(node, (int)idx);
      if (n == NULL) {
        cJSON_AddItemToArray(node, n = cJSON_CreateObject());
      } else if (cJSON_IsNull(n)) {
        cJSON_ReplaceItemInArray(node, (int)idx, n = cJSON_CreateObject());
      }
      node = n;
    } else {
      free(path_copy);
      return NULL;
    }

    token = strtok_r(NULL, "/", &saveptr);
  }

  free(path_copy);
  return node;
}

err_t xjson_make_type(const xjson root,
                      const char* path,
                      xjson_type_e type,
                      const void* value) {
  if (root == NULL || path == NULL) return X_RET_INVAL;

  // "/foo/bar/baz" -> "/foo/bar"
  char* parent = xjson_dirname(path);
  if (parent == NULL) return X_RET_ERROR;

  // "/foo/bar/baz" -> "baz"
  char* last_token = xjson_basename(path);
  if (last_token == NULL) {
    free(parent);
    return X_RET_ERROR;
  }

  // make all the parent nodes
  // if make error, return
  xjson last_node = xjson_make_parent(root, parent);
  if (last_node == NULL) {
    free(parent);
    free(last_token);
    return X_RET_ERROR;
  }

  // check if the last token KV exists
  xjson last_children = cJSON_GetObjectItem(last_node, last_token);
  if (cJSON_IsNull(last_children)) {
    cJSON_DeleteItemFromObject(last_node, last_token);
  } else if (last_children) {
    free(parent);
    free(last_token);
    return X_RET_EXIST;
  }

  switch (type) {
    case XJSON_TYPE_STRING:
      cJSON_AddStringToObject(last_node, last_token, (const char*)value);
      break;
    case XJSON_TYPE_NUMBER:
      cJSON_AddNumberToObject(last_node, last_token, *(const double*)value);
      break;
    case XJSON_TYPE_BOOLEAN:
      cJSON_AddBoolToObject(last_node, last_token, *(const xbool_t*)value);
      break;
    case XJSON_TYPE_NULL:
      cJSON_AddNullToObject(last_node, last_token);
      break;
    case XJSON_TYPE_ARRAY:
      cJSON_AddArrayToObject(last_node, last_token);
      break;
    case XJSON_TYPE_OBJECT:
      cJSON_AddObjectToObject(last_node, last_token);
      break;
    default:
      return X_RET_ERROR;
  }

  free(parent);
  free(last_token);

  return X_RET_OK;
}

err_t xjson_make_string(xjson root, const char* path, const char* string) {
  if (string == NULL) return X_RET_INVAL;

  return xjson_make_type(root, path, XJSON_TYPE_STRING, string);
}

err_t xjson_make_number(xjson root, const char* path, double number) {
  return xjson_make_type(root, path, XJSON_TYPE_NUMBER, &number);
}

err_t xjson_make_bool(xjson root, const char* path, xbool_t boolean) {
  return xjson_make_type(root, path, XJSON_TYPE_BOOLEAN, &boolean);
}

err_t xjson_make_true(xjson root, const char* path) {
  xbool_t value = xTRUE;
  return xjson_make_bool(root, path, value);
}
err_t xjson_make_false(xjson root, const char* path) {
  xbool_t value = xFALSE;
  return xjson_make_bool(root, path, value);
}

err_t xjson_make_null(xjson root, const char* path) {
  return xjson_make_type(root, path, XJSON_TYPE_NULL, NULL);
}

err_t xjson_makevf_type(xjson root,
                        const char* fmtpath,
                        xjson_type_e type,
                        const void* value,
                        va_list args) {
  char path[xjson_formats_max_length] = {0};
  vsnprintf(path, sizeof(path), fmtpath, args);
  return xjson_make_type(root, path, type, value);
}

err_t xjson_makef_string(xjson root,
                         const char* fmtpath,
                         const char* string,
                         ...) {
  if (string == NULL) return X_RET_INVAL;

  va_list args;
  va_start(args, string);
  err_t err = xjson_makevf_type(root, fmtpath, XJSON_TYPE_STRING, string, args);
  va_end(args);
  return err;
}

err_t xjson_makef_number(xjson root, const char* fmtpath, double number, ...) {
  va_list args;
  va_start(args, number);
  err_t err = xjson_makevf_type(root,
                                fmtpath,
                                XJSON_TYPE_NUMBER,
                                &number,
                                args);
  va_end(args);
  return err;
}

err_t xjson_makef_bool(xjson root, const char* fmtpath, int boolean, ...) {
  va_list args;
  va_start(args, boolean);
  err_t err = xjson_makevf_type(root,
                                fmtpath,
                                XJSON_TYPE_BOOLEAN,
                                &boolean,
                                args);
  va_end(args);
  return err;
}

err_t xjson_makef_true(xjson root, const char* fmtpath, ...) {
  xbool_t value = xTRUE;
  va_list args;
  va_start(args, fmtpath);
  err_t err = xjson_makevf_type(root,
                                fmtpath,
                                XJSON_TYPE_BOOLEAN,
                                &value,
                                args);
  va_end(args);
  return err;
}

err_t xjson_makef_false(xjson root, const char* fmtpath, ...) {
  xbool_t value = xFALSE;
  va_list args;
  va_start(args, fmtpath);
  err_t err = xjson_makevf_type(root,
                                fmtpath,
                                XJSON_TYPE_BOOLEAN,
                                &value,
                                args);
  va_end(args);
  return err;
}

err_t xjson_makef_null(xjson root, const char* fmtpath, ...) {
  va_list args;
  va_start(args, fmtpath);
  err_t err = xjson_makevf_type(root, fmtpath, XJSON_TYPE_NULL, NULL, args);
  va_end(args);
  return err;
}

xjson xjson_make_array(xjson root, const char* path) {
  err_t err = xjson_make_type(root, path, XJSON_TYPE_ARRAY, NULL);
  if (err != X_RET_OK) return NULL;

  return xjson_search(root, path);
}

xjson xjson_make_object(xjson root, const char* path) {
  return xjson_make_parent(root, path);
}

err_t xjson_set_type(xjson root,
                     const char* path,
                     xjson_type_e type,
                     const void* value) {
  if (root == NULL || path == NULL) return X_RET_INVAL;

  char* parent = xjson_dirname(path);
  if (parent == NULL) return X_RET_ERROR;

  char* last_token = xjson_basename(path);
  if (last_token == NULL) {
    free(parent);
    return X_RET_ERROR;
  }

  xjson father = xjson_search(root, parent);
  if (father == NULL || xjson_search(father, last_token) == NULL) {
    free(parent);
    free(last_token);
    return X_RET_NOTENT;
  }

  if (cJSON_IsObject(father)) {
    /* remove the last token item */
    cJSON_DeleteItemFromObject(father, last_token);

  } else if (cJSON_IsArray(father)) {
    char* endptr;
    int index = strtol(last_token, &endptr, 10);
    if (*endptr != '\0' || index < 0) {
      free(parent);
      free(last_token);
      return X_RET_ERROR;
    }
    cJSON_DeleteItemFromArray(father, index);
  } else {
    free(parent);
    free(last_token);
    return X_RET_ERROR;
  }

  switch (type) {
    case XJSON_TYPE_STRING:
      cJSON_AddItemToObject(father,
                            last_token,
                            cJSON_CreateString((const char*)value));
      break;
    case XJSON_TYPE_NUMBER:
      cJSON_AddItemToObject(father,
                            last_token,
                            cJSON_CreateNumber(*(double*)value));
      break;
    case XJSON_TYPE_BOOLEAN:
      cJSON_AddItemToObject(father,
                            last_token,
                            cJSON_CreateBool(*(xbool_t*)value));
      break;
    case XJSON_TYPE_NULL:
      cJSON_AddItemToObject(father, last_token, cJSON_CreateNull());
      break;
    default:
      free(parent);
      free(last_token);
      return X_RET_ERROR;
  }

  free(parent);
  free(last_token);

  return X_RET_OK;
}

err_t xjson_set_string(xjson root, const char* path, const char* string) {
  if (string == NULL) return X_RET_INVAL;

  return xjson_set_type(root, path, XJSON_TYPE_STRING, string);
}

err_t xjson_set_number(xjson root, const char* path, double number) {
  return xjson_set_type(root, path, XJSON_TYPE_NUMBER, &number);
}
err_t xjson_set_bool(xjson root, const char* path, xbool_t boolean) {
  return xjson_set_type(root, path, XJSON_TYPE_BOOLEAN, &boolean);
}

err_t xjson_set_true(xjson root, const char* path) {
  return xjson_set_bool(root, path, xTRUE);
}

err_t xjson_set_false(xjson root, const char* path) {
  return xjson_set_bool(root, path, xFALSE);
}

err_t xjson_set_null(xjson root, const char* path) {
  return xjson_set_type(root, path, XJSON_TYPE_NULL, NULL);
}

void xjson_debug(const xjson j, const char* prefix) {
  char* string = xjson_to_string(j);
  if (!string) return;

  if (prefix == NULL)
    XLOG_D("%s", string);
  else
    XLOG_D("%s %s", prefix, string);

  free(string);
}

void xjson_debug_pretty(const xjson j, const char* prefix) {
  char* string = xjson_to_string_pretty(j);
  if (!string) return;

  if (prefix == NULL)
    XLOG_D("%s", string);
  else
    XLOG_D("%s %s", prefix, string);

  free(string);
}

const char* xjson_get_string(const xjson root, const char* path) {
  return xjson_query_string(root, path, "");
}

int xjson_get_int(const xjson root, const char* path) {
  return xjson_query_int(root, path, 0);
}

double xjson_get_double(const xjson root, const char* path) {
  return xjson_query_double(root, path, 0.0);
}

xbool_t xjson_get_bool(const xjson root, const char* path) {
  return xjson_query_bool(root, path, xFALSE);
}

xjson xjson_get_object(const xjson root, const char* path) {
  return xjson_query_object(root, path, NULL);
}

xjson xjson_get_array(const xjson j, const char* path) {
  return xjson_query_array(j, path, NULL);
}

const char* xjson_query_string(const xjson root,
                               const char* path,
                               const char* default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson found = xjson_search(root, path);
  if (found == NULL || !cJSON_IsString(found)) return default_value;

  return cJSON_GetStringValue(found);
}

int xjson_query_int(const xjson root, const char* path, int default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson found = xjson_search(root, path);
  if (found == NULL || !cJSON_IsNumber(found)) return default_value;

  return cJSON_GetNumberValue(found);
}

double xjson_query_double(const xjson root,
                          const char* path,
                          double default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson found = xjson_search(root, path);
  if (found == NULL || !cJSON_IsNumber(found)) return default_value;

  return cJSON_GetNumberValue(found);
}

xbool_t xjson_query_bool(const xjson root,
                         const char* path,
                         xbool_t default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson found = xjson_search(root, path);
  if (found == NULL || !cJSON_IsBool(found)) return default_value;

  if (cJSON_IsTrue(found))
    return xTRUE;
  else
    return xFALSE;
}

xjson xjson_query_object(const xjson root,
                         const char* path,
                         xjson default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson retval = xjson_search(root, path);
  if (retval == NULL || !cJSON_IsObject(retval)) return default_value;

  return retval;
}

xjson xjson_query_array(const xjson root,
                        const char* path,
                        xjson default_value) {
  if (root == NULL || path == NULL) return default_value;

  xjson found = xjson_search(root, path);
  if (found == NULL || !cJSON_IsArray(found)) return default_value;

  return found;
}

err_t xjson_upsert_string(xjson root, const char* path, const char* string) {
  if (string == NULL) return X_RET_INVAL;

  err_t err = xjson_make_string(root, path, string);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_string(root, path, string);
}

err_t xjson_upsert_number(xjson root, const char* path, double number) {
  err_t err = xjson_make_number(root, path, number);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_number(root, path, number);
}

err_t xjson_upsert_bool(xjson root, const char* path, xbool_t boolean) {
  err_t err = xjson_make_bool(root, path, boolean);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_bool(root, path, boolean);
}

err_t xjson_upsert_true(xjson root, const char* path) {
  err_t err = xjson_make_bool(root, path, xTRUE);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_bool(root, path, xTRUE);
}

err_t xjson_upsert_false(xjson root, const char* path) {
  err_t err = xjson_make_bool(root, path, xFALSE);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_bool(root, path, xFALSE);
}

err_t xjson_upsert_null(xjson root, const char* path) {
  err_t err = xjson_make_null(root, path);
  if (err == X_RET_OK) return X_RET_OK;

  return xjson_set_null(root, path);
}

err_t xjson_replace(xjson root, const char* path, xjson value) {
  if (root == NULL || path == NULL || value == NULL) {
    return X_RET_INVAL;
  }

  xjson parent = NULL;
  xjson found = xjson_search_and_return_parent(root, path, &parent);
  if (found == NULL || parent == NULL) return X_RET_NOTENT;

  if (cJSON_IsObject(parent)) {
    cJSON_ReplaceItemInObject(parent, found->string, value);
  } else if (cJSON_IsArray(parent)) {
    int which = 0;
    cJSON* t = NULL;
    cJSON_ArrayForEach(t, parent) {
      if (t == found) {
        break;
      }
      which++;
    }

    if (t == found)
      cJSON_ReplaceItemInArray(parent, which, value);
    else
      return X_RET_NOTENT;
  } else {
    return X_RET_ERROR;
  }

  return X_RET_OK;
}
