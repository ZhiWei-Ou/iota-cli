/**
 * @brief 字符串
 * @file xstring.h
 * @author Oswin
 * @date 2025-12-06
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XLT_XSTRING_H_
#define XLT_XSTRING_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h>

#include "xdef.h"

/** @brief Flag for case-sensitive operations. */
#define X_CASE (0)
/** @brief Flag for case-insensitive operations. */
#define X_NOCASE (1)

/** @brief Default capacity for small string optimization. */
#ifndef X_STRING_DEFAULT_CAP
#define X_STRING_DEFAULT_CAP (32)
#endif /* X_STRING_DEFAULT_CAP */

/**
 * @brief The core dynamic string structure.
 *
 * It uses a small string optimization (SSO). If the string length is less than
 * X_STRING_DEFAULT_CAP, it is stored in the internal buffer `b`. Otherwise,
 * a buffer is allocated on the heap and pointed to by `s`.
 */
typedef struct {
  int len; /**< Current length of the string. */
  int cap; /**< Current capacity of the allocated buffer. */
  char* s; /**< Pointer to heap-allocated buffer for large strings. */
  char b[X_STRING_DEFAULT_CAP]; /**< Internal buffer for small strings. */
} xstring;

/**
 * @brief Initializes an empty xstring.
 * @return An empty xstring instance.
 * @example
 * xstring s = xstring_init_empty();
 * // s is now an empty string
 * xstring_free(&s);
 */
#define xstring_init_empty(void)   \
  ((xstring){                      \
      .len = 0,                    \
      .cap = X_STRING_DEFAULT_CAP, \
      .s = NULL,                   \
      .b = {0},                    \
  })

/**
 * @brief Initializes an xstring from a C-style string literal.
 * @param str The input C-style string.
 * @return A new xstring instance containing a copy of str.
 * @example
 * xstring s = xstring_init_iter("hello");
 * // s now contains "hello"
 * xstring_free(&s);
 */
xstring xstring_init_iter(const char* str);
void xstring_init_iter_r(xstring* s, const char* str);

/**
 * @brief Initializes an xstring from another xstring.
 * @param s The input xstring.
 * @return A new xstring instance containing a copy of s.
 * @example
 * xstring s1 = xstring_init_iter("hello");
 * xstring s2 = xstring_init_from_other(&s1);
 * // s2 now contains "hello"
 * xstring_free(&s1);
 * xstring_free(&s2);
 */
xstring xstring_init_from_other(const xstring* s);
void xstring_init_from_other_r(xstring* s1, const xstring* s2);

/**
 * @brief Initializes an xstring using a printf-style format.
 * @param fmt The format string.
 * @param ... Arguments for the format string.
 * @return A new xstring instance containing the formatted string.
 * @example
 * xstring s = xstring_init_format("value: %d", 42);
 * // s now contains "value: 42"
 * xstring_free(&s);
 */
xstring xstring_init_format(const char* fmt, ...);
void xstring_init_format_r(xstring* s, const char* fmt, ...);
/**
 * @brief Initializes an xstring using a printf-style format.
 * @param fmt The format string.
 * @param args The arguments for the format string.
 * @return A new xstring instance containing the formatted string.
 * @example
 * xstring s = xstring_init_vformat("value: %d", ap);
 * // s now contains "value: 42"
 * xstring_free(&s);
 */
xstring xstring_init_vformat(const char* fmt, va_list ap);
void xstring_init_vformat_r(xstring* s, const char* fmt, va_list ap);

/**
 * @brief Frees the memory used by an xstring.
 * @param s A pointer to the xstring to free.
 * @example
 * xstring s = xstring_init_iter("some string");
 * xstring_free(&s);
 */
void xstring_free(xstring* s);

/** @brief Macro for xstring_equal_ex. Compares two strings for equality. */
#define xstring_equal(s1, s2, f) xstring_equal_ex(s1, s2, f)
/** @brief Macro for xstring_has_charset_ex. Checks if a string contains any
 * character from a given set. */
#define xstring_has_charset(s, cs, f) xstring_has_charset_ex(s, cs, f)
/** @brief Macro for xstring_has_substr_ex. Checks if a string contains a given
 * substring. */
#define xstring_has_substr(s, ss, f) xstring_has_substr_ex(s, ss, f)
/** @brief Macro for xstring_has_prefix_ex. Checks if a string starts with a
 * given prefix. */
#define xstring_has_prefix(s, ps, f) xstring_has_prefix_ex(s, ps, f)
/** @brief Macro for xstring_has_suffix_ex. Checks if a string ends with a given
 * suffix. */
#define xstring_has_suffix(s, ss, f) xstring_has_suffix_ex(s, ss, f)

/**
 * @brief Gets the character at a specific index.
 * @param s The xstring.
 * @param index The 0-based index of the character to get.
 * @return The character at the specified index, or 0 if the index is out of bounds.
 * @example
 * xstring s = xstring_init_iter("world");
 * char c = xstring_at(&s, 1); // c will be 'o'
 * xstring_free(&s);
 */
char xstring_at(const xstring* s, int index);

/**
 * @brief Gets a pointer to a specific position within the string.
 * @param s The xstring.
 * @param index The 0-based index to point to.
 * @return A const char pointer to the character at the index, or NULL if out of bounds.
 * @example
 * xstring s = xstring_init_iter("world");
 * const char* p = xstring_start(&s, 2); // p will point to "rld"
 * xstring_free(&s);
 */
const char* xstring_start(const xstring* s, int index);

/**
 * @brief Gets the underlying C-style string.
 * @param s The xstring.
 * @return A const char pointer to the null-terminated string.
 * @example
 * xstring s = xstring_init_iter("hello");
 * printf("%s\n", xstring_to_string(&s)); // prints "hello"
 * xstring_free(&s);
 */
const char* xstring_to_string(const xstring* s);

/**
 * @brief Gets the length of the string.
 * @param s The xstring.
 * @return The length of the string (number of characters).
 * @example
 * xstring s = xstring_init_iter("hello");
 * int len = xstring_length(&s); // len will be 5
 * xstring_free(&s);
 */
int xstring_length(const xstring* s);

/**
 * @brief Gets the capacity of the string.
 * @param s The xstring.
 * return The capacity of the string (number of characters).
 * @example
 * xstring s = xstring_init_iter("hello");
 * int cap = xstring_capcity(&s); // cap will be 32
 * xstring_free(&s);
 *
 * xstring s2 = xstring_init_iter(....); // long long string
 * int cap2 = xstring_capcity(&s2); // cap2 will be strlen(s2) + 1
 * xstring_free(&s2);
 */
int xstring_capcity(const xstring* s);

/**
 * @brief Clears the string, making it empty.
 * @param s A pointer to the xstring to clear.
 * @example
 * xstring s = xstring_init_iter("hello");
 * xstring_clear(&s); // s is now empty
 * xstring_free(&s);
 */
void xstring_clear(xstring* s);

/**
 * @brief Trims whitespace from both ends of the string.
 * @param s A pointer to the xstring to trim.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("  hello  ");
 * xstring_trim(&s); // s now contains "hello"
 * xstring_free(&s);
 */
const char* xstring_trim(xstring* s);

/**
 * @brief Trims whitespace from the beginning (left side) of the string.
 * @param s A pointer to the xstring to trim.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("  hello  ");
 * xstring_trim_left(&s); // s now contains "hello  "
 * xstring_free(&s);
 */
const char* xstring_trim_left(xstring* s);

/**
 * @brief Trims whitespace from the end (right side) of the string.
 * @param s A pointer to the xstring to trim.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("  hello  ");
 * xstring_trim_right(&s); // s now contains "  hello"
 * xstring_free(&s);
 */
const char* xstring_trim_right(xstring* s);

/**
 * @brief Checks if the string is empty.
 * @param s The xstring.
 * @return xTRUE if the string is empty, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_empty();
 * xbool_t is_empty = xstring_is_empty(&s); // is_empty will be xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_is_empty(const xstring* s);

/**
 * @brief Concatenates a C-style string to an xstring.
 * @param s A pointer to the destination xstring.
 * @param str The C-style string to append.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("hello ");
 * xstring_cat(&s, "world"); // s now contains "hello world"
 * xstring_free(&s);
 */
const char* xstring_cat(xstring* s, const char* str);

/**
 * @brief Prepends a C-style string to an xstring.
 * @param s A pointer to the destination xstring.
 * @param str The C-style string to prepend.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("world");
 * xstring_prepend(&s, "hello "); // s now contains "hello world"
 * xstring_free(&s);
 */
const char* xstring_prepend(xstring* s, const char* str);

/**
 * @brief Converts the string to uppercase.
 * @param s The xstring to modify.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("Hello");
 * xstring_upper(&s); // s now contains "HELLO"
 * xstring_free(&s);
 */
const char* xstring_upper(const xstring* s);

/**
 * @brief Converts the string to lowercase.
 * @param s The xstring to modify.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("Hello");
 * xstring_lower(&s); // s now contains "hello"
 * xstring_free(&s);
 */
const char* xstring_lower(const xstring* s);

/**
 * @brief Tokenizes a string by a set of characters (thread-safe).
 * @param s The xstring to tokenize. This parameter is only used to get the base string and is not modified.
 * @param charset The set of delimiter characters.
 * @param token In/Out parameter. On input, `*token` should point to the current position in the string to search from.
 *              On output, `*token` is updated to point to the start of the
 * found token.
 * @return The length of the found token. Returns 0 if no token is found.
 * @note This function is re-entrant. The caller is responsible for managing the cursor.
 * @example
 * xstring s = xstring_init_iter("a,b;c");
 * const char* cursor = xstring_to_string(&s);
 * const char* end = cursor + xstring_length(&s);
 * const char* tok;
 * int len;
 * while(cursor < end) {
 *   tok = cursor;
 *   len = xstring_tokenize_by_charset(&s, ",;", &tok);
 *   if (len == 0 && *tok == '\0') break;
 *   printf("Token: %.*s\n", len, tok);
 *   cursor = tok + len;
 *   cursor += strspn(cursor, ",;");
 * }
 * xstring_free(&s);
 */
int xstring_tokenize_by_charset(const xstring* s,
                                const char* charset,
                                const char** token);
/**
 * @brief Tokenizes a string by a substring delimiter (thread-safe).
 * @param s The xstring to tokenize.
 * @param substr The delimiter substring.
 * @param token In/Out parameter. `*token` points to the start of the string segment to search.
 * @return The length of the token found before the delimiter.
 * @note This function is re-entrant. The caller is responsible for managing the cursor.
 * @example
 * xstring s = xstring_init_iter("part1--part2");
 * const char* cursor = xstring_to_string(&s);
 * const char* tok = cursor;
 * int len = xstring_tokenize_by_substr(&s, "--", &tok);
 * // len is 5, tok points to "part1"
 * xstring_free(&s);
 */
int xstring_tokenize_by_substr(const xstring* s,
                               const char* substr,
                               const char** token);

/**
 * @brief Replaces all occurrences of a substring with another substring.
 * @param s A pointer to the xstring to modify.
 * @param old_str The substring to be replaced.
 * @param new_str The substring to replace with.
 * @return A pointer to the beginning of the modified string data.
 * @example
 * xstring s = xstring_init_iter("abab");
 * xstring_replace(&s, "ab", "c"); // s now contains "cc"
 * xstring_free(&s);
 */
const char* xstring_replace(xstring* s,
                            const char* old_str,
                            const char* new_str);

/**
 * @brief Checks if a string contains any character from a given set.
 * @param s The xstring to check.
 * @param charset The set of characters to look for.
 * @param flag `X_CASE` for case-sensitive, `X_NOCASE` for case-insensitive.
 * @return xTRUE if a character is found, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_iter("hello");
 * xbool_t has = xstring_has_charset_ex(&s, "abc", X_CASE); // has is xFALSE
 * xbool_t has2 = xstring_has_charset_ex(&s, "xyz", X_NOCASE); // has2 is xFALSE
 * xbool_t has3 = xstring_has_charset_ex(&s, "e", X_CASE); // has3 is xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_has_charset_ex(const xstring* s, const char* charset, int flag);

/**
 * @brief Checks if a string contains a given substring.
 * @param s The xstring to check.
 * @param substr The substring to look for.
 * @param flag `X_CASE` for case-sensitive, `X_NOCASE` for case-insensitive.
 * @return xTRUE if the substring is found, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_iter("HelloWorld");
 * xbool_t has = xstring_has_substr_ex(&s, "world", X_CASE); // has is xFALSE
 * xbool_t has2 = xstring_has_substr_ex(&s, "world", X_NOCASE); // has2 is xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_has_substr_ex(const xstring* s, const char* substr, int flag);

/**
 * @brief Checks if a string starts with a given prefix.
 * @param s The xstring to check.
 * @param prefix The prefix to check for.
 * @param flag `X_CASE` for case-sensitive, `X_NOCASE` for case-insensitive.
 * @return xTRUE if the string starts with the prefix, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_iter("HelloWorld");
 * xbool_t has = xstring_has_prefix_ex(&s, "hello", X_CASE); // has is xFALSE
 * xbool_t has2 = xstring_has_prefix_ex(&s, "hello", X_NOCASE); // has2 is xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_has_prefix_ex(const xstring* s, const char* prefix, int flag);

/**
 * @brief Checks if a string ends with a given suffix.
 * @param s The xstring to check.
 * @param suffix The suffix to check for.
 * @param flag `X_CASE` for case-sensitive, `X_NOCASE` for case-insensitive.
 * @return xTRUE if the string ends with the suffix, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_iter("HelloWorld");
 * xbool_t has = xstring_has_suffix_ex(&s, "World", X_CASE); // has is xTRUE
 * xbool_t has2 = xstring_has_suffix_ex(&s, "world", X_NOCASE); // has2 is xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_has_suffix_ex(const xstring* s, const char* suffix, int flag);

/**
 * @brief Compares an xstring with a C-style string for equality.
 * @param s1 The xstring.
 * @param s2 The C-style string.
 * @param flag `X_CASE` for case-sensitive, `X_NOCASE` for case-insensitive.
 * @return xTRUE if the strings are equal, xFALSE otherwise.
 * @example
 * xstring s = xstring_init_iter("Hello");
 * xbool_t eq = xstring_equal_ex(&s, "hello", X_CASE); // eq is xFALSE
 * xbool_t eq2 = xstring_equal_ex(&s, "hello", X_NOCASE); // eq2 is xTRUE
 * xstring_free(&s);
 */
xbool_t xstring_equal_ex(const xstring* s1, const char* s2, int flag);

/**
 * @brief Converts a string to an integer.
 * @param s The xstring to convert.
 * @param base The number base (e.g., 10 for decimal, 16 for hex).
 * @return The converted integer value. Returns 0 if conversion fails.
 * @example
 * xstring s = xstring_init_iter("123");
 * int val = xstring_stoi(&s, 10); // val is 123
 * xstring_free(&s);
 */
int xstring_stoi(const xstring* s, int base);

/**
 * @brief Converts a string to a double.
 * @param s The xstring to convert.
 * @return The converted double value. Returns 0.0 if conversion fails.
 * @example
 * xstring s = xstring_init_iter("123.45");
 * double val = xstring_stod(&s); // val is 123.45
 * xstring_free(&s);
 */
double xstring_stod(const xstring* s);

/**
 * @brief Converts an integer to a string.
 * @param val The integer value to convert.
 * @return A new xstring containing the string representation of the integer.
 * @example
 * xstring s = xstring_itos(123);
 * // s contains "123"
 * xstring_free(&s);
 */
xstring xstring_itos(int val);

/**
 * @brief Converts a double to a string.
 * @param val The double value to convert.
 * @return A new xstring containing the string representation of the double.
 * @example
 * xstring s = xstring_dtos(123.45);
 * // s contains "123.450000" (default precision)
 * xstring_free(&s);
 */
xstring xstring_dtos(double val);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XLT_XSTRING_H_ */
