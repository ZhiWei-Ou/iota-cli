/**
 * @brief 选项解析器
 * @file xoption.h
 * @author Oswin
 * @date 2025-12-01
 * @details A modern and lightweight command-line option parsing library.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef X_LITE_TOOLS_xoption_H_
#define X_LITE_TOOLS_xoption_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"
#include "xlist.h"

/** @brief Opaque handle to an xoption instance. */
typedef struct xoption_priv* xoption;
/** @brief Opaque handle to an xoption candidate instance. */
typedef struct xoption_candidate_priv* xoption_candidate;

/** @brief Function type for xoption actions. */
typedef void (*xoption_action_fn)(xoption context, void* user_data);
/** @brief Function type for xoption runner. */
typedef err_t (*xoption_run_fn)(xoption context);

/**
 * @brief Creates a new xoption instance.
 * @return A handle to the new xoption instance.
 */
xoption xoption_create_root();

/**
 * @brief Creates a new xoption subcommand.
 *  This function will add the subcommand to the parent xoption @ref root
 *  instance.
 * @param root The parent xoption instance.
 * @param name The name of the subcommand.
 * @param desc The description of the subcommand.
 * @return A handle to the new xoption subcommand.
 */
xoption xoption_create_subcommand(xoption root,
                                  const char* name,
                                  const char* desc);

/**
 * @brief Sets the user context for an xoption instance.
 * This context pointer can be accessed in action functions and hooks.
 * @param self The xoption instance.
 * @param context The user context pointer.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoption_set_context(xoption self, void* context);

/**
 * @brief Gets the user context for an xoption instance.
 * This context pointer can be accessed in action functions and hooks.
 * @param self The xoption instance.
 * @return The user context pointer.
 */
void* xoption_get_context(xoption self);

/**
 * @brief Sets the prefix and suffix prompts for an xoption instance.
 *  These prompts are printed before and after parsing, respectively.
 * @param self The xoption instance.
 * @param prompt The prefix prompt.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoption_set_prefix_prompt(xoption self, const char* prompt);

/**
 * @brief Sets the suffix prompt for an xoption instance.
 *  This prompt is printed after parsing.
 * @param self The xoption instance.
 * @param prompt The suffix prompt.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoption_set_suffix_prompt(xoption self, const char* prompt);

/**
 * @brief Adds a post-parse callback to an xoption instance.
 *
 * This callback is invoked **after parsing completes successfully**.
 * The return value of this callback will be used as the return value of @ref
 * `xoption_parse()`.
 *
 * @param self The xoption instance.
 * @param callback The callback function to be called after parsing.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoption_set_post_parse_callback(xoption self, xoption_run_fn callback);

/**
 * @brief Helper printf for option parsing context (formatted output).
 * @param self The xoption instance providing output context.
 * @param prompt_fmt Printf-style format string.
 */
#define xoption_helper_printf(self, prompt_fmt, ...) \
  xoption_helper_printf_advance(self, xTRUE, xTRUE, prompt_fmt, ##__VA_ARGS__)

/**
 * @brief Helper printf for option parsing context (formatted output).
 * @param self The xoption instance providing output context.
 * @param whether_print_prefix Whether to print the prefix prompt.
 *        required prefix has been set, \ref xoption_set_prefix_prompt().
 * @param whether_print_suffix Whether to print the suffix prompt.
 *        required suffix has been set, \ref xoption_set_suffix_prompt().
 * @param prompt_fmt Printf-style format string.
 */
void xoption_helper_printf_advance(xoption self,
                                   xbool_t whether_print_prefix,
                                   xbool_t whether_print_suffix,
                                   const char* prompt_fmt,
                                   ...);

/**
 * @brief Finalizes parsing with optional help/error printing and exits flow.
 *  Convenience function used by internal parsing logic and actions to
 *  terminate processing. If `print_help` is true, help text will be
 *  printed before returning. `error_fmt` is an optional printf-style
 *  format used to print an error message.
 * @param self The xoption instance.
 * @param print_help Whether to print help text.
 * @param error_fmt Optional printf-style error format string.
 */
void xoption_done(xoption self, xbool_t print_help, const char* error_fmt, ...);

/**
 * @brief Disables helper functions for xoption.
 *  This function will remove the 'h' and the 'help' options.
 * @param self The xoption instance.
 * @return X_RET_OK  on success.
 */
err_t xoption_disable_default_hepler(xoption self);

/**
 * @brief Returns the list of positional (non-option) arguments.
 *  This list is ordered from first to last.
 * @param self The xoption instance.
 * @return An `xlist` containing positional argument strings in order.
 */
xlist xoption_get_positional(xoption self);

/**
 * @brief Parses command line arguments using the configured options.
 *  This function runs registered prehooks, parses `argc/argv`,
 *  dispatches actions or populates bound variables, then runs
 *  posthooks. If parsing fails, `xoption_done` may be called to
 *  display help or error messages.
 * @param self The xoption instance.
 * @param argc Argument count from `main`.
 * @param argv Argument vector from `main`.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoption_parse(xoption self, int argc, char* argv[]);

/**
 * @brief Adds a string option candidate to the parser.
 *  Creates an option that expects a string value and stores the
 *  parsed value into `ptr` (ownership remains with the caller).
 * @param self The xoption instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param hint A short hint describing the expected value (e.g., "FILE").
 * @param desc Description of the option for help output.
 * @param ptr Pointer to a `char*` which will receive the parsed string.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoption_candidate, or NULL on failure.
 */
xoption_candidate xoption_add_string(xoption self,
                                     char sn,
                                     const char* ln,
                                     const char* hint,
                                     const char* desc,
                                     char** ptr,
                                     xbool_t required);

/**
 * @brief Adds a numeric option candidate to the parser.
 *  Creates an option that expects an integer value and stores the
 *  parsed value into `ptr`.
 * @param self The xoption instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param hint A short hint describing the expected value (e.g., "N").
 * @param desc Description of the option for help output.
 * @param ptr Pointer to an `int` which will receive the parsed number.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoption_candidate, or NULL on failure.
 */
xoption_candidate xoption_add_number(xoption self,
                                     char sn,
                                     const char* ln,
                                     const char* hint,
                                     const char* desc,
                                     int* ptr,
                                     xbool_t required);

/**
 * @brief Adds a boolean flag candidate to the parser.
 *  Boolean options do not take an explicit value. When present,
 *  they set the pointed `ptr` to true (non-zero).
 * @param self The xoption instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param desc Description of the option for help output.
 * @param ptr Pointer to an `xbool_t` which will be updated when the flag is present.
 * @return A handle to the created xoption_candidate, or NULL on failure.
 */
xoption_candidate xoption_add_boolean(
    xoption self, char sn, const char* ln, const char* desc, xbool_t* ptr);

/**
 * @brief Adds an action option candidate to the parser.
 *  Action options trigger a callback when encountered during parsing
 *  instead of storing a value. The `user_data` pointer is passed to
 *  the callback when invoked.
 * @param self The xoption instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param desc Description of the option for help output.
 * @param fn The callback to invoke when the option is seen.
 * @param user_data A pointer passed to the callback function.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoption_candidate, or NULL on failure.
 */
xoption_candidate xoption_add_action(xoption self,
                                     char sn,
                                     const char* ln,
                                     const char* desc,
                                     xoption_action_fn fn,
                                     void* user_data);

/**
 * @brief Checks whether an option candidate was provided by the user.
 *  This function returns true if the option was present on the command
 * @param self The xoption_candidate handle to query.
 * @return Non-zero (true) if the candidate was used/present on the command line,
 *         zero (false) otherwise.
 */
xbool_t xoption_candidate_is_used(xoption_candidate self);

/**
 * @brief Destroys an xoption instance and frees associated resources.
 *  This will unregister any subcommands and free internal data
 *  structures allocated by the xoption implementation.
 * @param self The xoption instance to destroy.
 * @return X_RET_OK on success.
 */
err_t xoption_destroy(xoption self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* X_LITE_TOOLS_xoption_H_ */
