/**
 * @brief 结构化命令行标志解析
 * @file xflag.h
 * @author Oswin
 * @date 2025-06-26
 * @details A simple and lightweight command-line flag parsing library.
 * It allows defining flags of different types (integer, boolean, string) and
 * parsing them from `argv`.
 * @see test/unix/xflag_example.cpp and test/unix/xflag_test.cpp
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef XTOOL_XFLAG__H_
#define XTOOL_XFLAG__H_

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a flag set instance.
 */
typedef struct xflag_priv* xflag;

/**
 * @brief Creates a new, empty flag set.
 * @return A handle to the new flag set, or NULL on failure.
 */
xflag xflag_create();

/**
 * @brief Enables the default description and help flag (-h, --help).
 * @param self The flag set.
 * @return X_RET_OK  on success.
 */
err_t xflag_enable_default_description(xflag self);

/**
 * @brief Sets the program name, which is used in the help message.
 * @param self The flag set.
 * @param name The name of the program.
 * @return X_RET_OK  on success.
 */
err_t xflag_set_program_name(xflag self, const char* name);

/**
 * @brief Parses the command-line arguments.
 * @details It processes `argv` and populates the variables associated with the registered flags.
 * @param self The flag set.
 * @param argc The argument count from main().
 * @param argv The argument vector from main().
 * @return X_RET_OK  on success, or an error code if parsing fails (e.g., unknown flag).
 */
err_t xflag_parse(xflag self, int argc, char** argv);

/**
 * @brief Destroys a flag set and frees all associated memory.
 * @param self The flag set to destroy.
 * @return X_RET_OK  on success.
 */
err_t xflag_destroy(xflag self);

/**
 * @brief Retrieves the last error message that occurred during parsing.
 * @param self The flag set.
 * @return A constant string describing the error, or NULL if no error occurred.
 */
const char* xflag_error(xflag self);

/**
 * @brief Registers an integer flag.
 * @param self The flag set.
 * @param short_name The short name of the flag (e.g., "p" for -p). Can be NULL.
 * @param long_name The long name of the flag (e.g., "port" for --port). Can be NULL.
 * @param description The help text for this flag.
 * @param value A pointer to an integer variable that will be updated with the flag's value when parsed.
 * @return X_RET_OK  on success.
 */
err_t xflag_add_int(xflag self,
                    const char* short_name,
                    const char* long_name,
                    const char* description,
                    int* value);

/**
 * @brief Registers a boolean flag.
 * @details Boolean flags are treated as presence flags. If the flag (e.g., --verbose) is present, the value is set to true.
 * @param self The flag set.
 * @param short_name The short name of the flag (e.g., "v" for -v). Can be NULL.
 * @param long_name The long name of the flag (e.g., "verbose" for --verbose). Can be NULL.
 * @param description The help text for this flag.
 * @param value A pointer to an xbool_t variable that will be set to `xTRUE` if the flag is present.
 * @return X_RET_OK  on success.
 */
err_t xflag_add_bool(xflag self,
                     const char* short_name,
                     const char* long_name,
                     const char* description,
                     xbool_t* value);

/**
 * @brief Registers a string flag.
 * @param self The flag set.
 * @param short_name The short name of the flag (e.g., "f" for -f). Can be NULL.
 * @param long_name The long name of the flag (e.g., "file" for --file). Can be NULL.
 * @param description The help text for this flag.
 * @param value A pointer to a `char*` variable that will be updated to point to the flag's argument string. The string is owned by argv.
 * @return X_RET_OK  on success.
 */
err_t xflag_add_string(xflag self,
                       const char* short_name,
                       const char* long_name,
                       const char* description,
                       char** value);

/**
 * @brief Function pointer for a custom flag handler.
 * @param d User-provided data, passed through from `xflag_add_handler`.
 */
typedef void (*xflag_handler)(void* d);

/**
 * @brief Registers a flag that triggers a callback function.
 * @details Useful for flags that perform an immediate action, like --version or --help.
 * @param self The flag set.
 * @param short_name The short name of the flag.
 * @param long_name The long name of the flag.
 * @param description The help text for this flag.
 * @param f The handler function to call when the flag is encountered.
 * @param d An optional user data pointer to be passed to the handler function.
 * @return X_RET_OK  on success.
 */
err_t xflag_add_handler(xflag self,
                        const char* short_name,
                        const char* long_name,
                        const char* description,
                        xflag_handler f,
                        void* d);

#ifdef __cplusplus
}
#endif

#endif /* XTOOL_XFLAG__H_ */
