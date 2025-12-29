/**
 * @brief 通用定义
 * @file xdef.h
 * @author Oswin
 * @date 2025-06-25
 * @details Provides common macro definitions and standardized error codes
 *        shared throughout the codebase.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XDEF__H_
#define XTOOL_XDEF__H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @name The xbox hook function prototypes
 * @{
 */

/** @brief Prototype for memory allocation. */
typedef void* (*malloc_fn)(size_t size);
/** @brief Prototype for zero-initialized memory allocation. */
typedef void* (*calloc_fn)(size_t nmemb, size_t size);
/** @brief Prototype for re-allocating memory. */
typedef void* (*realloc_fn)(void* ptr, size_t size);
/** @brief Prototype for freeing memory. */
typedef void (*free_fn)(void* ptr);
/** @brief Prototype for program termination. */
typedef void (*exit_fn)(int status);

/** @} */

/** @struct xbox_hook_t
 *  @brief A structure to hold hooks for memory and exit functions.
 */
typedef struct {
  /**< Malloc function hook. */
  malloc_fn malloc;
  /**< Calloc function hook. */
  calloc_fn calloc;
  /**< Realloc function hook. */
  realloc_fn realloc;
  /**< Free function hook. */
  free_fn free;
  /**< Exit function hook. */
  exit_fn exit;
} xbox_hook_t;

/**
 * @brief Initializes the hooks for memory and exit functions.
 * @param hook A pointer to the xbox_hook_t structure.
 */
void xbox_init_hooks(const xbox_hook_t* hook);

/** @brief Standardized error type. */
typedef int err_t;
/** @brief Standardized boolean type. */
typedef int8_t xbool_t;

/** @brief Represents a false boolean value. */
#define xFALSE 0
/** @brief Represents a true boolean value. */
#define xTRUE 1

/**
 * @brief Converts a boolean value to a string.
 * @param b The boolean value.
 * @return A string representation of the boolean value.
 */
const char* xbool_str(xbool_t b);

/** @brief Standardized error codes. */
enum {
  X_RET_OK = 0,         /**< Non-error */
  X_RET_ERROR = -1,     /**< Generic error */
  X_RET_INVAL = -2,     /**< Invalid argument */
  X_RET_FULL = -3,      /**< Full */
  X_RET_EXIST = -4,     /**< Exists */
  X_RET_NOMEM = -5,     /**< Memory allocation error */
  X_RET_NOTENT = -6,    /**< Not found */
  X_RET_TIMEOUT = -7,   /**< Timeout */
  X_RET_NOTSUP = -8,    /**< Not supported */
  X_RET_EMPTY = -9,     /**< Empty */
  X_RET_MISMATCH = -10, /**< Mismatch */
  X_RET_BADFMT = -11,   /**< Bad format */
  X_RET_OVERFLOW = -12, /**< Overflow */
};

/**
 * @brief Converts an error code to a string.
 * @param err The error code.
 * @return A string representation of the error code.
 */
const char* err_str(err_t err);

/** @brief Calculates the number of elements in an array. */
#define xARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/** @brief Checks if a number is odd. */
#define xODD(x) ((x) & 1)
/** @brief Checks if a number is even. */
#define xEVEN(x) (!(x & 1))

/** @brief Returns the minimum of two values. */
#define xMIN(a, b) ((a) < (b) ? (a) : (b))
/** @brief Returns the maximum of two values. */
#define xMAX(a, b) ((a) > (b) ? (a) : (b))

/** @brief Converts a macro argument to a string literal. */
#define xMACRO_STRING(str) (#str)
/** @brief Expands a macro argument and then converts it to a string literal. */
#define XMACRO_EXPAND_STRING(str) xMACRO_STRING(str)

/** @brief Suppresses unused variable warnings. */
#define xUNUSED(x) ((void)(x))

#if defined(__GNUC__) || defined(__clang__)
#define xWEAK __attribute__((weak))
#elif defined(_MSC_VER)
#define xWEAK __declspec(selectany)
#else
#define xWEAK
#endif

// #define XBOX_ENABLE_BACKTRACE
#if defined(XBOX_ENABLE_BACKTRACE)
/** @brief Wrapper for malloc with backtrace. */
#define xbox_malloc(size) __xbox_malloc_backtrace(__FILE__, __LINE__, size)
/** @brief Wrapper for calloc with backtrace. */
#define xbox_calloc(count, size) \
  __xbox_calloc_backtrace(__FILE__, __LINE__, count, size)
/** @brief Wrapper for realloc with backtrace. */
#define xbox_realloc(ptr, size) \
  __xbox_realloc_backtrace(__FILE__, __LINE__, ptr, size)
/** @brief Wrapper for free with backtrace. */
#define xbox_free(ptr) __xbox_free_backtrace(__FILE__, __LINE__, ptr)
/** @brief Wrapper for exit with backtrace. */
#define xbox_exit(code) __xbox_exit_backtrace(__FILE__, __LINE__, code)
#else
/** @brief Standard memory allocation wrapper. */
#define xbox_malloc(size) __xbox_malloc(size)
/** @brief Standard memory allocation wrapper for an array. */
#define xbox_calloc(count, size) __xbox_calloc(count, size)
/** @brief Standard memory reallocation wrapper. */
#define xbox_realloc(ptr, size) __xbox_realloc(ptr, size)
/** @brief Standard memory free wrapper. */
#define xbox_free(ptr) __xbox_free(ptr)
/** @brief Standard exit wrapper. */
#define xbox_exit(code) __xbox_exit(code)
#endif /* XBOX_ENABLE_BACKTRACE */

/**
 * @brief Duplicates a string.
 * @param str The string to duplicate.
 * @return A pointer to the newly allocated string, or NULL on failure.
 */
char* xbox_strdup(const char* str);

/**
 * @brief Formats and allocates a string.
 * @param strp A pointer to a char* that will hold the new string.
 * @param fmt The format string.
 * @param ... The arguments for the format string.
 * @return The number of characters written, or -1 on failure.
 */
int xbox_asprintf(char** strp, const char* fmt, ...);

/**
 * @brief Formats and allocates a string using a va_list.
 * @param strp A pointer to a char* that will hold the new string.
 * @param fmt The format string.
 * @param ap The va_list of arguments.
 * @return The number of characters written, or -1 on failure.
 */
int xbox_vasprintf(char** strp, const char* fmt, va_list ap);

/**
 * @internal
 * @brief Internal memory allocation function.
 * @param __s Size of memory to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* __xbox_malloc(size_t __s);
/**
 * @internal
 * @brief Internal memory allocation function for an array.
 * @param __c Number of elements.
 * @param __s Size of each element.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* __xbox_calloc(size_t __c, size_t __s);
/**
 * @internal
 * @brief Internal memory reallocation function.
 * @param __p Pointer to memory to reallocate.
 * @param __s New size of memory.
 * @return Pointer to reallocated memory, or NULL on failure.
 */
void* __xbox_realloc(void* __p, size_t __s);
/**
 * @internal
 * @brief Internal memory free function.
 * @param ptr Pointer to memory to free.
 */
void __xbox_free(void* ptr);
/**
 * @internal
 * @brief Internal exit function.
 * @param code Exit code.
 */
void __xbox_exit(int code);

#if defined(XBOX_ENABLE_BACKTRACE)
/**
 * @internal
 * @brief Internal memory allocation function with backtrace.
 * @param __f File name where allocation occurred.
 * @param __l Line number where allocation occurred.
 * @param __s Size of memory to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* __xbox_malloc_backtrace(const char* __f, int __l, size_t __s);
/**
 * @internal
 * @brief Internal memory allocation function for an array with backtrace.
 * @param __f File name where allocation occurred.
 * @param __l Line number where allocation occurred.
 * @param __c Number of elements.
 * @param __s Size of each element.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* __xbox_calloc_backtrace(const char* __f, int __l, size_t __c, size_t __s);
/**
 * @internal
 * @brief Internal memory reallocation function with backtrace.
 * @param __f File name where reallocation occurred.
 * @param __l Line number where reallocation occurred.
 * @param __p Pointer to memory to reallocate.
 * @param __s New size of memory.
 * @return Pointer to reallocated memory, or NULL on failure.
 */
void* __xbox_realloc_backtrace(const char* __f, int __l, void* __p, size_t __s);
/**
 * @internal
 * @brief Internal memory free function with backtrace.
 * @param __f File name where deallocation occurred.
 * @param __l Line number where deallocation occurred.
 * @param __p Pointer to memory to free.
 */
void __xbox_free_backtrace(const char* __f, int __l, void* __p);
/**
 * @internal
 * @brief Internal exit function with backtrace.
 * @param __f File name where exit occurred.
 * @param __l Line number where exit occurred.
 * @param __c Exit code.
 */
void __xbox_exit_backtrace(const char* __f, int __l, int __c);
#endif /* XBOX_ENABLE_BACKTRACE */

#ifdef __cplusplus
}
#endif

#endif /* XTOOL_XDEF__H_ */
