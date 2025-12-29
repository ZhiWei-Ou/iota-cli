/**
 * @brief 文件解析组件
 * @file parser.c
 * @author Oswin
 * @date 2025-09-08
 * @details Utility functions for parsing from and saving to standard and encrypted JSON files.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "parser.h"

#include <string.h>

#include "aes.h"
#include "os_file.h"

// clang-format off
static const uint8_t security_key[16] = {0xAF, 0x26, 0xD3, 0x51, 0x9E, 0x72,
                                         0x68, 0xC0, 0x1B, 0x4F, 0x87, 0x22,
                                         0xFA, 0x3D, 0xCC, 0x19};
static const uint8_t security_iv[16] = {0x8D, 0x34, 0x6F, 0xA2, 0x51, 0x09,
                                        0xBC, 0x73, 0x28, 0xE5, 0x47, 0xDA,
                                        0x10, 0xC9, 0xFE, 0x65};
// clang-format on

static xjson parser_from_json_file(const char* file, xbool_t security) {
  if (file == NULL) return NULL;

  xjson object = NULL;
  const char* ext = os_file_extname(file);
  uint8_t* buf = os_file_readall(file);
  if (buf == NULL) return xjson_create();

  if (security) {
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, security_key, security_iv);
    AES_CTR_xcrypt_buffer(&ctx, buf, os_file_size(file));
  }

  if (strcmp(ext, ".jsonc") == 0) {
    object = xjson_parse_has_comments((const char*)buf);
  } else {
    object = xjson_parse((const char*)buf);
  }

  xbox_free(buf);

  return object;
}

xjson parser_from_json(const char* file) {
  return parser_from_json_file(file, xFALSE);
}

xjson parser_from_security_json(const char* file) {
  return parser_from_json_file(file, xTRUE);
}

static err_t parser_save_to_json_file(const char* file,
                                      const xjson json,
                                      xbool_t security) {
  if (file == NULL || json == NULL) return X_RET_INVAL;

  char* text = xjson_to_string(json);
  if (text == NULL) return X_RET_ERROR;

  size_t text_len = strlen(text);

  if (security) {
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, security_key, security_iv);
    AES_CTR_xcrypt_buffer(&ctx, (uint8_t*)text, text_len);
  }

  os_file_write(file, (const uint8_t*)text, text_len);

  xbox_free(text);

  return X_RET_OK;
}

err_t parser_save_json(const xjson json, const char* file) {
  return parser_save_to_json_file(file, json, xFALSE);
}

err_t parser_save_security_json(const xjson json, const char* file) {
  return parser_save_to_json_file(file, json, xTRUE);
}
