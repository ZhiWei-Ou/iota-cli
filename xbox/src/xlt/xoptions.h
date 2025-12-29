/**
 * @brief 选项解析器
 * @file xoptions.h
 * @author Oswin
 * @date 2025-12-01
 * @details A modern and lightweight command-line option parsing library.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef X_LITE_TOOLS_XOPTIONS_H_
#define X_LITE_TOOLS_XOPTIONS_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xdef.h"
#include "xlist.h"

/** @brief Opaque handle to an xoptions instance. */
typedef struct xoptions_priv* xoptions;
/** @brief Opaque handle to an xoptions candidate instance. */
typedef struct xoptions_candidate_priv* xoptions_candidate;

/** @brief Function type for xoptions actions. */
typedef void (*xoptions_action_fn)(xoptions context, void* user_data);
/** @brief Function type for xoptions hooks. */
typedef void (*xoptions_action_hook_fn)(xoptions context);

/**
 * @brief Creates a new xoptions instance.
 * @return A handle to the new xoptions instance.
 */
xoptions xoptions_create_root();

/**
 * @brief Creates a new xoptions subcommand.
 *  This function will add the subcommand to the parent xoptions @ref root
 *  instance.
 * @param root The parent xoptions instance.
 * @param name The name of the subcommand.
 * @param desc The description of the subcommand.
 * @return A handle to the new xoptions subcommand.
 */
xoptions xoptions_create_subcommand(xoptions root,
                                    const char* name,
                                    const char* desc);

/**
 * @brief Sets the prefix and suffix prompts for an xoptions instance.
 *  These prompts are printed before and after parsing, respectively.
 * @param self The xoptions instance.
 * @param prompt The prefix prompt.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoptions_set_prefix_prompt(xoptions self, const char* prompt);

/**
 * @brief Sets the suffix prompt for an xoptions instance.
 *  This prompt is printed after parsing.
 * @param self The xoptions instance.
 * @param prompt The suffix prompt.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoptions_set_suffix_prompt(xoptions self, const char* prompt);

/**
 * @brief Adds a pre-parse hook to an xoptions instance.
 *  Prehook functions are executed before parsing begins. They can be
 *  used to initialize state, adjust option defaults, or register
 *  additional candidates dynamically.
 * @param self The xoptions instance.
 * @param prehook The hook function to be called before parsing.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoptions_set_prehook(xoptions self, xoptions_action_hook_fn prehook);

/**
 * @brief Adds a post-parse hook to an xoptions instance.
 *  Posthook functions are executed after parsing completes. They can
 *  be used to validate parsed values or perform actions dependent on
 *  parsed options.
 * @param self The xoptions instance.
 * @param posthook The hook function to be called after parsing.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoptions_set_posthook(xoptions self, xoptions_action_hook_fn posthook);

/**
 * @brief Helper printf for option parsing context (formatted output).
 * @param self The xoptions instance providing output context.
 * @param prompt_fmt Printf-style format string.
 */
#define xoptions_helper_printf(self, prompt_fmt, ...) \
  xoptions_helper_printf_advance(self, xTRUE, xTRUE, prompt_fmt, ##__VA_ARGS__)

/**
 * @brief Helper printf for option parsing context (formatted output).
 * @param self The xoptions instance providing output context.
 * @param whether_print_prefix Whether to print the prefix prompt.
 *        required prefix has been set, \ref xoptions_set_prefix_prompt().
 * @param whether_print_suffix Whether to print the suffix prompt.
 *        required suffix has been set, \ref xoptions_set_suffix_prompt().
 * @param prompt_fmt Printf-style format string.
 */
void xoptions_helper_printf_advance(xoptions self,
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
 * @param self The xoptions instance.
 * @param print_help Whether to print help text.
 * @param error_fmt Optional printf-style error format string.
 */
void xoptions_done(xoptions self,
                   xbool_t print_help,
                   const char* error_fmt,
                   ...);

/**
 * @brief Disables helper functions for xoptions.
 *  This function will remove the 'h' and the 'help' options.
 * @param self The xoptions instance.
 * @return X_RET_OK  on success.
 */
err_t xoptions_disable_default_hepler(xoptions self);

/**
 * @brief Returns the list of positional (non-option) arguments.
 *  This list is ordered from first to last.
 * @param self The xoptions instance.
 * @return An `xlist` containing positional argument strings in order.
 */
xlist xoptions_get_positional(xoptions self);

/**
 * @brief Parses command line arguments using the configured options.
 *  This function runs registered prehooks, parses `argc/argv`,
 *  dispatches actions or populates bound variables, then runs
 *  posthooks. If parsing fails, `xoptions_done` may be called to
 *  display help or error messages.
 * @param self The xoptions instance.
 * @param argc Argument count from `main`.
 * @param argv Argument vector from `main`.
 * @return X_RET_OK on success, or an error code on failure.
 */
err_t xoptions_parse(xoptions self, int argc, char* argv[]);

/**
 * @brief Adds a string option candidate to the parser.
 *  Creates an option that expects a string value and stores the
 *  parsed value into `ptr` (ownership remains with the caller).
 * @param self The xoptions instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param hint A short hint describing the expected value (e.g., "FILE").
 * @param desc Description of the option for help output.
 * @param ptr Pointer to a `char*` which will receive the parsed string.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoptions_candidate, or NULL on failure.
 */
xoptions_candidate xoptions_add_string(xoptions self,
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
 * @param self The xoptions instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param hint A short hint describing the expected value (e.g., "N").
 * @param desc Description of the option for help output.
 * @param ptr Pointer to an `int` which will receive the parsed number.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoptions_candidate, or NULL on failure.
 */
xoptions_candidate xoptions_add_number(xoptions self,
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
 * @param self The xoptions instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param desc Description of the option for help output.
 * @param ptr Pointer to an `xbool_t` which will be updated when the flag is present.
 * @return A handle to the created xoptions_candidate, or NULL on failure.
 */
xoptions_candidate xoptions_add_boolean(
    xoptions self, char sn, const char* ln, const char* desc, xbool_t* ptr);

/**
 * @brief Adds an action option candidate to the parser.
 *  Action options trigger a callback when encountered during parsing
 *  instead of storing a value. The `user_data` pointer is passed to
 *  the callback when invoked.
 * @param self The xoptions instance.
 * @param sn Short name (single character) for the option, or 0 if none.
 * @param ln Long name (string) for the option, or NULL if none.
 * @param desc Description of the option for help output.
 * @param fn The callback to invoke when the option is seen.
 * @param user_data A pointer passed to the callback function.
 * @param required Whether the option is required (non-zero means required).
 * @return A handle to the created xoptions_candidate, or NULL on failure.
 */
xoptions_candidate xoptions_add_action(xoptions self,
                                       char sn,
                                       const char* ln,
                                       const char* desc,
                                       xoptions_action_fn fn,
                                       void* user_data);

/**
 * @brief Checks whether an option candidate was provided by the user.
 *  This function returns true if the option was present on the command
 * @param self The xoptions_candidate handle to query.
 * @return Non-zero (true) if the candidate was used/present on the command line,
 *         zero (false) otherwise.
 */
xbool_t xoptions_candidate_is_used(xoptions_candidate self);

/**
 * @brief Destroys an xoptions instance and frees associated resources.
 *  This will unregister any subcommands and free internal data
 *  structures allocated by the xoptions implementation.
 * @param self The xoptions instance to destroy.
 * @return X_RET_OK on success.
 */
err_t xoptions_destroy(xoptions self);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* X_LITE_TOOLS_XOPTIONS_H_ */
