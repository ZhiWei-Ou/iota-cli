/**
 * @brief 文件解析组件
 * @file parser.h
 * @author Oswin
 * @date 2025-09-08
 * @details Provides utility functions for parsing from and saving to
 *           standard and encrypted JSON files.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XFRAMEWORK_PARSER__H_
#define XFRAMEWORK_PARSER__H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xjson.h"

/**
 * @brief Reads a file and parses its content as a JSON object.
 * @param file The path to the JSON file.
 * @return A new xjson object on success, or NULL on failure (e.g., file not found, parse error).
 * @note The caller is responsible for freeing the returned xjson object using `xjson_delete()`.
 */
xjson parser_from_json(const char* file);

/**
 * @brief Reads an encrypted file, decrypts it, and parses the content as a JSON object.
 * @param file The path to the encrypted JSON file.
 * @return A new xjson object on success, or NULL on failure (e.g., file not found, decryption failed, parse error).
 * @note The caller is responsible for freeing the returned xjson object using `xjson_delete()`.
 */
xjson parser_from_security_json(const char* file);

/**
 * @brief Serializes a JSON object and saves it to a file.
 * @param json The xjson object to save.
 * @param file The path to the destination file. The file will be overwritten if it exists.
 * @return X_RET_OK  on success, or an error code on failure.
 */
err_t parser_save_json(const xjson json, const char* file);

/**
 * @brief Serializes a JSON object, encrypts it, and saves the encrypted content to a file.
 * @param json The xjson object to serialize and encrypt.
 * @param file The path to the destination file. The file will be overwritten if it exists.
 * @return X_RET_OK  on success, or an error code on failure.
 */
err_t parser_save_security_json(const xjson json, const char* file);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XFRAMEWORK_PARSER__H_ */
