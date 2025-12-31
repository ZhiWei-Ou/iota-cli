/**
 * @brief JSON 解析器
 * @file xjson.h
 * @author Oswin
 * @date 2025-06-26
 * @details A wrapper for the cJSON library that adds JSON Pointer (RFC 6901)
 *           functionality and provides a simplified API for creating, parsing,
 *           manipulating, and querying JSON objects.
 * @see https://datatracker.ietf.org/doc/html/rfc6901
 * @see test/unix/xjson_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XJSON__H_
#define XTOOL_XJSON__H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"
#include "xdef.h"

/**
 * @brief An alias for a cJSON object handle (cJSON*).
 */
typedef struct cJSON* xjson;

/**
 * @brief Initializes the cJSON library with custom memory allocation hooks.
 *        This should be called before any other xjson or cJSON function if you
 *        want to override the default malloc/free.
 * @param __malloc A function pointer to a custom malloc implementation.
 * @param __free A function pointer to a custom free implementation.
 */
void xjson_init_hooks(malloc_fn __malloc, free_fn __free);

/**
 * @brief Enumeration of JSON value types.
 */
typedef enum {
  XJSON_TYPE_STRING = 0X1,  /**< A string value. */
  XJSON_TYPE_NUMBER = 0X2,  /**< A number value (integer or double). */
  XJSON_TYPE_BOOLEAN = 0X4, /**< A boolean value (true or false). */
  XJSON_TYPE_NULL = 0X8,    /**< A null value. */
  XJSON_TYPE_ARRAY = 0X10,  /**< An array container. */
  XJSON_TYPE_OBJECT = 0X20, /**< An object container. */

  /** A bitmask for all scalar types (string, number, boolean, null). */
  XJSON_TYPE_SCALAR = XJSON_TYPE_STRING | XJSON_TYPE_NUMBER |
                      XJSON_TYPE_BOOLEAN | XJSON_TYPE_NULL,
  /** A bitmask for all composite types (array, object). */
  XJSON_TYPE_COMPOSITE = XJSON_TYPE_ARRAY | XJSON_TYPE_OBJECT,

  XJSON_TYPE_UNKOWN = 0x80 /**< An unknown or invalid type. */
} xjson_type_e;

/**
 * @brief Get the type of a JSON object.
 * @param json The JSON object to inspect.
 * @return The xjson_type_e enumeration value.
 */
xjson_type_e xjson_type(const xjson json);

/**
 * @brief Get the string representation of a JSON type.
 * @param tp The JSON type enumeration.
 * @return A constant string representing the type (e.g., "string", "number").
 */
const char* xjson_type_to_string(xjson_type_e tp);

/**
 * @defgroup XJsonTypeCheck Type Checking Functions
 * @brief Functions to check if a JSON object is of a specific type.
 * @{
 */
/** @brief Checks if the JSON object is a string. */
xbool_t xjson_is_string(const xjson);
/** @brief Checks if the JSON object is a number. */
xbool_t xjson_is_number(const xjson);
/** @brief Checks if the JSON object is a boolean. */
xbool_t xjson_is_boolean(const xjson);
/** @brief Checks if the JSON object is null. */
xbool_t xjson_is_null(const xjson);
/** @brief Checks if the JSON object is an array. */
xbool_t xjson_is_array(const xjson);
/** @brief Checks if the JSON object is an object. */
xbool_t xjson_is_object(const xjson);
/** @brief Checks if the JSON object is a scalar type. */
xbool_t xjson_is_scalar(const xjson);
/** @brief Checks if the JSON object is a composite type. */
xbool_t xjson_is_composite(const xjson);
/** @} */

/**
 * @brief Macro for generating a function name to marshal a C struct into a JSON object.
 *        Usage: `xjson_marshal(mytype, &my_struct_instance);` expands to
 * `xjson_marshal_mytype(&my_struct_instance);`
 */
#ifndef xjson_marshal
#define xjson_marshal(type, container) \
  xjson_marshal_##type((const void*)container)
#endif /* xjson_marshal */

/**
 * @brief Macro for generating a function name to unmarshal a JSON object into a C struct.
 *        Usage: `xjson_unmarshal(json, mytype, &my_struct_instance);` expands
 * to `xjson_unmarshal_mytype(json, &my_struct_instance);`
 */
#ifndef xjson_unmarshal
#define xjson_unmarshal(json, type, container) \
  xjson_unmarshal_##type(json, (void*)container)
#endif /* xjson_unmarshal */

/**
 * @brief Parses a JSON-formatted string into a JSON object.
 * @param string The null-terminated JSON string.
 * @return A new xjson object, or NULL on parsing error. The caller must free this with xjson_delete().
 */
xjson xjson_parse(const char* string);

/**
 * @brief Parses a JSON-formatted string that allows comments.
 * @param string The null-terminated JSON string with comments.
 * @return A new xjson object, or NULL on parsing error. The caller must free this with xjson_delete().
 */
xjson xjson_parse_has_comments(const char* string);

/**
 * @brief Creates a deep copy of a JSON object.
 * @param other The JSON object to duplicate.
 * @return A new xjson object. The caller must free this with xjson_delete().
 */
xjson xjson_duplicate(const xjson other);

/**
 * @brief Deletes a JSON object and frees all associated memory.
 * @param j The JSON object to delete.
 */
void xjson_delete(xjson j);

/** Aliases for xjson_delete for convenience. */
#ifndef xjson_free
#define xjson_free(j) xjson_delete(j)
#endif /* xjson_free */
#ifndef xjson_destroy
#define xjson_destroy(j) xjson_delete(j)
#endif /* xjson_destroy */

/**
 * @brief Converts a JSON object to a compact string.
 * @param json The JSON object.
 * @return A newly allocated string. The caller must free this with the appropriate free function from cJSON's hooks.
 */
#define xjson_to_string(json) xjson_to_string_advance(json, xFALSE)

/**
 * @brief Converts a JSON object to a formatted (pretty-printed) string.
 * @param json The JSON object.
 * @return A newly allocated string. The caller must free this with the appropriate free function from cJSON's hooks.
 */
#define xjson_to_string_pretty(json) xjson_to_string_advance(json, xTRUE)

/**
 * @brief Converts a JSON object to a string with selectable formatting.
 * @param j The JSON object.
 * @param is_pretty If true, the output is formatted with indentation.
 * @return A newly allocated string. The caller must free it.
 */
char* xjson_to_string_advance(const xjson j, xbool_t is_pretty);

/**
 * @brief Renders a JSON object into a user-provided buffer (re-entrant) as a compact string.
 */
#define xjson_to_string_r(json, buffer, buffer_size) \
  xjson_to_string_advance_r(json, buffer, buffer_size, xFALSE, "")

/**
 * @brief Renders a JSON object into a user-provided buffer (re-entrant) as a pretty-printed string.
 */
#define xjson_to_string_pretty_r(json, buffer, buffer_size) \
  xjson_to_string_advance_r(json, buffer, buffer_size, xTRUE, "")

/**
 * @brief Renders a JSON object into a user-provided buffer with selectable formatting.
 * @param j The JSON object.
 * @param buffer The user-provided buffer to write to.
 * @param buffer_size The size of the buffer.
 * @param is_pretty If true, format the output.
 * @param error_retval The value to return on error (e.g., if buffer is too small).
 * @return A pointer to the beginning of the buffer on success, or `error_retval` on failure.
 */
const char* xjson_to_string_advance_r(const xjson j,
                                      char* buffer,
                                      size_t buffer_size,
                                      xbool_t is_pretty,
                                      const char* error_retval);

/** @internal Helper macro for creating JSON objects with optional values. */
#define _XJSON_GET_CREATE(_0, _1, _create_function, ...) _create_function
xjson xjson_create_string_with_value(const char* string);
xjson xjson_create_string_empty(void);
xjson xjson_create_number_with_value(double number);
xjson xjson_create_zero_number(void);
xjson xjson_create_boolean_with_value(xbool_t boolean);
xjson xjson_create_false(void);

/**
 * @brief Creates a new, empty JSON object. Default create function.
 */
#ifndef xjson_create
#define xjson_create() xjson_create_object()
#endif /* xjson_create */

xjson xjson_create_object(void);
xjson xjson_create_array(void);
xjson xjson_create_null(void);

/**
 * @brief Creates a JSON boolean.
 * @details Two calling styles:
 *   1. `xjson_create_boolean(xTRUE);` // Creates a true boolean.
 *   2. `xjson_create_boolean();`      // Creates a false boolean.
 */
#define xjson_create_boolean(...)                    \
  _XJSON_GET_CREATE(dummy,                           \
                    ##__VA_ARGS__,                   \
                    xjson_create_boolean_with_value, \
                    xjson_create_false)(__VA_ARGS__)

/**
 * @brief Creates a JSON number.
 * @details Two calling styles:
 *   1. `xjson_create_number(123.45);` // Creates a number with the given value.
 *   2. `xjson_create_number();`       // Creates a number with value 0.
 */
#define xjson_create_number(...)                    \
  _XJSON_GET_CREATE(dummy,                          \
                    ##__VA_ARGS__,                  \
                    xjson_create_number_with_value, \
                    xjson_create_zero_number)(__VA_ARGS__)

/**
 * @brief Creates a JSON string.
 * @details Two calling styles:
 *   1. `xjson_create_string("hello");` // Creates a string with the given
 * value.
 *   2. `xjson_create_string();`      // Creates an empty string.
 */
#define xjson_create_string(...)                    \
  _XJSON_GET_CREATE(dummy,                          \
                    ##__VA_ARGS__,                  \
                    xjson_create_string_with_value, \
                    xjson_create_string_empty)(__VA_ARGS__)

/**
 * @brief Searches for a JSON object using a JSON Pointer path.
 * @param root The root of the JSON document.
 * @param path The JSON Pointer path (e.g., "/a/b/0").
 * @return A pointer to the found JSON object on success, or NULL if not found. The returned pointer is owned by the `root` object and should not be deleted.
 */
xjson xjson_search(const xjson root, const char* path);

#ifndef xjson_formats_max_length
#define xjson_formats_max_length (128)
#endif /* xjson_formats_max_length */

/**
 * @defgroup XJsonMake Create-at-Path Functions
 * @brief Functions to create a new value at a specific JSON pointer path.
 *        These functions will fail if the target path already exists.
 * @{
 */
/** @brief Creates a string at a specific path. */
err_t xjson_make_string(xjson r, const char* p, const char* v);
/** @brief Creates a number at a specific path. */
err_t xjson_make_number(xjson r, const char* p, double v);
/** @brief Creates a boolean at a specific path. */
err_t xjson_make_bool(xjson r, const char* p, xbool_t v);
/** @brief Creates a true value at a specific path. */
err_t xjson_make_true(xjson r, const char* p);
/** @brief Creates a false value at a specific path. */
err_t xjson_make_false(xjson r, const char* p);
/** @brief Creates a null value at a specific path. */
err_t xjson_make_null(xjson r, const char* p);
/** @} */

/**
 * @defgroup XJsonMakeF Create-at-Formatted-Path Functions
 * @brief Functions to create a new value at a printf-style formatted JSON pointer path.
 *        These functions will fail if the target path already exists.
 * @{
 */
/** @brief Creates a string at a formatted path. */
err_t xjson_makef_string(xjson r, const char* fmt, const char* v, ...);
/** @brief Creates a number at a formatted path. */
err_t xjson_makef_number(xjson r, const char* fmt, double v, ...);
/** @brief Creates a boolean at a formatted path. */
err_t xjson_makef_bool(xjson r, const char* fmt, int v, ...);
/** @brief Creates a true value at a formatted path. */
err_t xjson_makef_true(xjson r, const char* fmt, ...);
/** @brief Creates a false value at a formatted path. */
err_t xjson_makef_false(xjson r, const char* fmt, ...);
/** @brief Creates a null value at a formatted path. */
err_t xjson_makef_null(xjson r, const char* fmt, ...);
/** @} */

/**
 * @defgroup XJsonMakeComposite Create Composite Types at Path
 * @brief Functions to create a new composite type (object or array) at a path.
 * @{
 */
/** @brief Creates an array at a specific path. */
xjson xjson_make_array(xjson r, const char* p);
/** @brief Creates an object at a specific path. */
xjson xjson_make_object(xjson r, const char* p);
/** @} */

/**
 * @defgroup XJsonSet Replace-at-Path Functions
 * @brief Functions to replace an existing value at a specific JSON pointer path.
 *        These functions will fail if the target path does not exist.
 * @{
 */
/** @brief Sets a string value at a specific path. */
err_t xjson_set_string(xjson r, const char* p, const char* v);
/** @brief Sets a number value at a specific path. */
err_t xjson_set_number(xjson r, const char* p, double v);
/** @brief Sets a boolean value at a specific path. */
err_t xjson_set_bool(xjson r, const char* p, xbool_t v);
/** @brief Sets a true value at a specific path. */
err_t xjson_set_true(xjson r, const char* p);
/** @brief Sets a false value at a specific path. */
err_t xjson_set_false(xjson r, const char* p);
/** @brief Sets a null value at a specific path. */
err_t xjson_set_null(xjson r, const char* p);
/** @} */

/**
 * @defgroup XJsonDebug Debugging Functions
 * @brief Functions to print a JSON object to the log for debugging.
 * @{
 */
/** @brief Prints a compact representation of a JSON object to the log. */
void xjson_debug(const xjson root, const char* prefix_nullable);
/** @brief Prints a pretty representation of a JSON object to the log. */
void xjson_debug_pretty(const xjson root, const char* prefix_nullable);
/** @} */

/**
 * @defgroup XJsonGet Get-Value-at-Path Functions
 * @brief Functions to get a value from a specific JSON pointer path.
 *        If the path is not found or the type mismatches, a default value (0,
 * NULL, false) is returned.
 * @{
 */
/** @brief Gets a string value from a specific path. */
const char* xjson_get_string(const xjson r, const char* p);
/** @brief Gets an integer value from a specific path. */
int xjson_get_int(const xjson r, const char* p);
/** @brief Gets a double value from a specific path. */
double xjson_get_double(const xjson r, const char* p);
/** @brief Gets a boolean value from a specific path. */
xbool_t xjson_get_bool(const xjson r, const char* p);
/** @brief Gets an object from a specific path. */
xjson xjson_get_object(const xjson r, const char* p);
/** @brief Gets an array from a specific path. */
xjson xjson_get_array(const xjson r, const char* p);
/** @} */

/**
 * @defgroup XJsonQuery Query-Value-at-Path Functions
 * @brief Functions to get a value from a specific JSON pointer path, with a user-specified default.
 * @{
 */
/** @brief Queries a string value from a specific path, returning a default if
 * not found. */
const char* xjson_query_string(const xjson r, const char* p, const char* def);
/** @brief Queries an integer value from a specific path, returning a default if
 * not found. */
int xjson_query_int(const xjson r, const char* p, int def);
/** @brief Queries a double value from a specific path, returning a default if
 * not found. */
double xjson_query_double(const xjson r, const char* p, double def);
/** @brief Queries a boolean value from a specific path, returning a default if
 * not found. */
xbool_t xjson_query_bool(const xjson r, const char* p, xbool_t def);
/** @brief Queries an object from a specific path, returning a default if not
 * found. */
xjson xjson_query_object(const xjson r, const char* p, xjson def);
/** @brief Queries an array from a specific path, returning a default if not
 * found. */
xjson xjson_query_array(const xjson r, const char* p, xjson def);
/** @} */

/**
 * @defgroup XJsonUpsert Upsert-at-Path Functions
 * @brief Functions to "update or insert" a value at a specific JSON pointer path.
 *        These functions will create the value if the path does not exist, or
 * replace it if it does.
 * @{
 */
/** @brief Upserts a string value at a specific path. */
err_t xjson_upsert_string(xjson r, const char* p, const char* string);
/** @brief Upserts a number value at a specific path. */
err_t xjson_upsert_number(xjson r, const char* p, double number);
/** @brief Upserts a boolean value at a specific path. */
err_t xjson_upsert_bool(xjson r, const char* p, xbool_t boolean);
/** @brief Upserts a true value at a specific path. */
err_t xjson_upsert_true(xjson r, const char* p);
/** @brief Upserts a false value at a specific path. */
err_t xjson_upsert_false(xjson r, const char* p);
/** @brief Upserts a null value at a specific path. */
err_t xjson_upsert_null(xjson r, const char* p);
/** @} */

/**
 * @brief Replaces the item at `path` with a new `value` object.
 * @param root The root of the JSON document.
 * @param path The JSON pointer path to the item to be replaced.
 * @param value The new xjson object to insert. The parent object will take ownership of this value.
 * @return X_RET_OK  on success, or an error code on failure.
 */
err_t xjson_replace(xjson root, const char* path, xjson value);

/**
 * @brief A convenient alias for `cJSON_ArrayForEach`.
 * @details Usage:
 *   xjson item = NULL;
 *   xjson_foreach(item, array) {
 *     // process item
 *   }
 */
#ifndef xjson_foreach
#define xjson_foreach(json, root) cJSON_ArrayForEach(json, root)
#endif /* xjson_foreach */

/**
 * @brief A convenient alias for `cJSON_GetArraySize`.
 * @param json The JSON array object.
 * @return The number of items in the array.
 */
#ifndef xjson_array_size
#define xjson_array_size(json) cJSON_GetArraySize(json)
#endif /* xjson_array_size */

#ifdef __cplusplus
}
#endif

#endif /* XTOOL_XJSON__H_ */
