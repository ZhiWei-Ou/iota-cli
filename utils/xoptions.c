/**
 * @brief 选项解析器
 * @file xoptions.c
 * @author Oswin
 * @date 2025-12-01
 * @details A modern and lightweight command-line option parsing library.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#define XLOG_MOD "xoptions"
#include "xoptions.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xlog.h"

typedef enum {
  /**< short option. e.g. '-a' */
  XOPTIONS_KIND_SHORT,

  /**< long option. e.g. '--a' */
  XOPTIONS_KIND_LONG,

  /**< positional separator. e.g. '--' */
  XOPTIONS_KIND_POSITIONAL_SEPARATOR,

  /**< positional or subcommand. e.g. 'a' */
  XOPTIONS_KIND_POSITIONAL_OR_SUBCOMMAND,

  XOPTIONS_KIND_END
} xoptions_kind;
typedef enum {
  XOPTIONS_CANDIDATE_TYPE_SUBCOMMAND = 0x00,

  XOPTIONS_CANDIDATE_TYPE_OPTION = 0x10,
  XOPTIONS_CANDIDATE_TYPE_NUMBER = 0x11,
  XOPTIONS_CANDIDATE_TYPE_BOOLEAN = 0x12,
  XOPTIONS_CANDIDATE_TYPE_STRING = 0x14,
  XOPTIONS_CANDIDATE_TYPE_ACTION = 0x18,

  XOPTIONS_CANDIDATE_TYPE_END = 0x20
} xoptions_candidate_type;

struct xoptions_candidate_priv {
  xoptions_candidate_type type; /**< the type of the candidate */
  char sn; /**< the short name of the option. if == '\0', it means this option
              does not provide a short form. */
  const char* ln;        /**< the long name of the option. if empty, it means
                            this option does not provide a long form */
  const char* hint;      /**< the hint of the option */
  const char* desc;      /**< the description of the option */
  xoptions subcommand;   /**< the subcommand */
  xoptions_action_fn fn; /**< the action function */
  void* storing_ptr;     /**< the pointer to store the value */
  xbool_t required;      /**< whether this option is required */
  xbool_t used;          /**< whether this option is used */
  xoptions context;      /**< the context of the option */
};

struct xoptions_priv {
  const char*
      name; /**< Name of the subcommand. The root command has no name. */
  const char* first_argument;        /**< the first argument, argv[0] */
  struct xlist_priv candidates;      /**< the candidates list */
  struct xlist_priv positional_args; /**< the positional arguments list */
  xbool_t done;                      /**< whether the parsing is done */
  xbool_t end_of_options; /**< whether the parsing is at the end of options,
                             e.g. all of arguments after '--' */
  xoptions_action_hook_fn prehook; /**< the prehook function, if exists, it will
                                      be called before the parsing */
  xoptions_action_hook_fn posthook; /**< the posthook function, if exists, it
                                       will be called after the parsing */
  err_t err;                        /**< the error code */
  const char* desc;                 /**< the description of the command */
  const char* prefix_prompt;        /**< the prefix prompt */
  const char* suffix_prompt;        /**< the help message */
};

static xoptions_candidate __candidate_new();
static void __candidate_free(xoptions_candidate self);
static void __candidate_print(xoptions_candidate self,
                              size_t left_width,
                              const char* front_padding,
                              const char* back_padding);
static err_t __candidate_assignment(xoptions_candidate self,
                                    const char* next,
                                    int* argv_index,
                                    xbool_t is_inline_arg);
static err_t __candidate_assignment_number(xoptions_candidate self,
                                           const char* next,
                                           int* argv_index,
                                           xbool_t is_inline_arg);
static err_t __candidate_assignment_boolean(xoptions_candidate self,
                                            const char* next,
                                            int* argv_index,
                                            xbool_t is_inline_arg);
static err_t __candidate_assignment_string(xoptions_candidate self,
                                           const char* next,
                                           int* argv_index,
                                           xbool_t is_inline_arg);
static err_t __candidate_assignment_action(xoptions_candidate self,
                                           const char* value);
static err_t __atoi(const char* value, int* out);
static xbool_t __is_short_option(const char* name, char** start);
static xbool_t __is_long_option(const char* name, char** start);
static xbool_t __is_positional_separator(const char* name);
static void __xoptions_default_helper_action(xoptions self, void* user_data);
static void __xoptions_check_required(xoptions self);
static void __xoptions_try_match_candidate(xoptions self,
                                           int* curr_index,
                                           int argc,
                                           char** argv);
static void __print_options(const xlist options);
static void __print_commands(const xlist commands);
static const char* __basename(const char* path);

/**
 * @brief Enables helper functions for xoptions.
 *  This function will call @ref xoptions_add_action to add the 'h' and
 *  the 'help' options.
 * @param self The xoptions instance.
 */
void xoptions_enable_default_hepler(xoptions self);

xoptions xoptions_create_root() {
  xoptions self = (xoptions)xbox_malloc(sizeof(struct xoptions_priv));
  if (!self) return NULL;

  self->name = "";
  self->first_argument = "";
  xlist_init(&self->candidates);
  xlist_init(&self->positional_args);
  self->done = xFALSE;
  self->end_of_options = xFALSE;
  self->prehook = NULL;
  self->posthook = NULL;
  self->err = X_RET_OK;
  self->desc = "";
  self->prefix_prompt = "";
  self->suffix_prompt = "";

  xoptions_enable_default_hepler(self);

  return self;
}

err_t xoptions_disable_default_hepler(xoptions self) {
  if (!self) return X_RET_INVAL;

  void* item;
  xlist_foreach(&self->candidates, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    if (candidate->sn == 'h' && strcmp(candidate->ln, "help") == 0) {
      xlist_remove(&self->candidates, candidate);
      __candidate_free(candidate);
      break;
    }
  }

  return X_RET_OK;
}

void xoptions_enable_default_hepler(xoptions self) {
  if (!self) return;

  xoptions_add_action(self,
                      'h',
                      "help",
                      "Display this help message.",
                      __xoptions_default_helper_action,
                      NULL);

  return;
}

xoptions xoptions_create_subcommand(xoptions root,
                                    const char* name,
                                    const char* desc) {
  xoptions self = xoptions_create_root();

  if (!self) return NULL;
  self->name = name;
  self->desc = desc;

  xoptions_candidate candidate = __candidate_new();
  if (!candidate) {
    xoptions_destroy(self);
    return NULL;
  }

  candidate->type = XOPTIONS_CANDIDATE_TYPE_SUBCOMMAND;
  candidate->subcommand = self;
  candidate->context = root;
  candidate->desc = desc;

  xlist_push_back(&root->candidates, candidate);

  return self;
}

err_t xoptions_set_prefix_prompt(xoptions self, const char* prompt) {
  if (!self || !prompt) return X_RET_INVAL;

  self->prefix_prompt = prompt;
  return X_RET_OK;
}

err_t xoptions_set_suffix_prompt(xoptions self, const char* prompt) {
  if (!self || !prompt) return X_RET_INVAL;

  self->suffix_prompt = prompt;
  return X_RET_OK;
}

err_t xoptions_set_prehook(xoptions self, xoptions_action_hook_fn prehook) {
  if (!self || !prehook) return X_RET_INVAL;

  self->prehook = prehook;
  return X_RET_OK;
}
err_t xoptions_set_posthook(xoptions self, xoptions_action_hook_fn posthook) {
  if (!self || !posthook) return X_RET_INVAL;

  self->posthook = posthook;
  return X_RET_OK;
}

err_t xoptions_destroy(xoptions self) {
  if (self == NULL) return X_RET_INVAL;

  void* data_ptr = NULL;
  for (data_ptr = xlist_pop_back(&self->candidates); data_ptr;
       data_ptr = xlist_pop_back(&self->candidates)) {
    __candidate_free((xoptions_candidate)data_ptr);
  }

  for (data_ptr = xlist_pop_back(&self->positional_args); data_ptr;
       data_ptr = xlist_pop_back(&self->positional_args)) {
  }

  xbox_free(self);

  return X_RET_OK;
}

static void __xoptions_try_match_candidate(xoptions self,
                                           int* curr_index,
                                           int argc,
                                           char** argv) {
  int index = *curr_index;
  const char* curr = argv[index]; /* the current const argument */
  const char* next = index + 1 < argc ? argv[index + 1]
                                      : NULL; /* the next const argument */
  xbool_t option_active = xFALSE;             // current option is active
  xbool_t is_inline_argument = xFALSE;        // e.g. --arg=123
  xbool_t is_combined_argument = xFALSE;  // e.g. -abc (the same as -a -b -c)

  xoptions_kind kind = XOPTIONS_KIND_END;
  char* pos = (char*)curr;
  size_t pos_len = strlen(pos);

  size_t buf_used = 0;
  char stack_buf[32] = {0};
  char* heap_buf = NULL;
  char* safe_buf = stack_buf;

  if (strlen(curr) > xARRAY_SIZE(stack_buf)) {
    heap_buf = xbox_malloc(strlen(curr) + 1);
    memset(heap_buf, 0, strlen(curr) + 1);
    safe_buf = heap_buf;
  }

  if (__is_short_option(pos, &pos)) {
    /* skip the '-', and then copy option name to 'safe_buf' */
    kind = XOPTIONS_KIND_SHORT;
    strcpy(safe_buf, pos);

    if (strlen(safe_buf) > 1) is_combined_argument = xTRUE;

  } else if (__is_long_option(pos, &pos)) {
    /* skip the '--', and then copy option name to 'safe_buf' */
    kind = XOPTIONS_KIND_LONG;
    strcpy(safe_buf, pos);
  } else if (__is_positional_separator(pos)) {
    /* the current argument is a positional separator
     * exit match candidate process.*/
    kind = XOPTIONS_KIND_POSITIONAL_SEPARATOR;
    self->end_of_options = xTRUE;
    goto exit;
  } else {
    /* the current argument is a positional argument or a subcommand.
     * copy the current argument to 'safe_buf' */
    kind = XOPTIONS_KIND_POSITIONAL_OR_SUBCOMMAND;
    strcpy(safe_buf, pos);
  }

  void* item;  // the candidate item

  char* eq = strchr(curr, '=');
  if (eq) {
    /* split the safe_buf, such as '-abc=123' to '-abc\0123'
     * subsequently, safe_buf will be directly used for matching,
     * The '=' sign and the subsequent string will be replaced by the "hulu"
     * character.*/
    if (strchr(safe_buf, '=')) strchr(safe_buf, '=')[0] = '\0';

    // set the next argument as inline argument
    // 'next' is a constant pointer
    next = eq + 1;
    is_inline_argument = xTRUE;
  }

  /* use current option 'safe_buf' to check all candidates. */
  xlist_foreach(&self->candidates, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;

    /* Exit early if an error occurred or parsing is already complete,
   e.g., when encountering an action like '-h' or '--help'. */
    if (self->done == xTRUE) break;

    if (kind == XOPTIONS_KIND_SHORT) {
      const char* tmp_ptr = safe_buf;

      /* check combined form, such as '-abc'.
       * use 'a' 'b' 'c' to check */
      xbool_t found = xFALSE;
      while (*tmp_ptr) {
        if (candidate->sn == *tmp_ptr) {
          found = xTRUE;
          break;
        }
        tmp_ptr++;
      }

      if (found) {
        option_active = xTRUE;

        /* '=' is not allowed for short options;
         * use space to separate the value instead. */
        if (is_inline_argument) {
          xoptions_done(self,
                        xTRUE,
                        "error: option '-%c' does not accept '=' form ('-%s' "
                        "is "
                        "invalid).\n",
                        candidate->sn,
                        safe_buf);
          break;
        }

        /* only boolean short options can be combined (e.g., -abc). */
        if (is_combined_argument &&
            candidate->type != XOPTIONS_CANDIDATE_TYPE_BOOLEAN) {
          // clang-format off
            xoptions_done(self, xTRUE,
                          "option '-%c' requires a value and cannot be used in a combined form: '-%s'\n",
                          candidate->sn, safe_buf);
          // clang-format on
          break;
        } else if (is_combined_argument &&
                   candidate->type == XOPTIONS_CANDIDATE_TYPE_BOOLEAN) {
          /* set next to NULL.
           * For example, in cases like '-abc true', do not treat 'true' as a
           * short option argument.
           */
          __candidate_assignment(candidate, NULL, curr_index, xFALSE);
        } else {
          /* support '-a' == '-a true' */
          __candidate_assignment(candidate, next, curr_index, xFALSE);
        }
      }  // end if found

    } else if (kind == XOPTIONS_KIND_LONG) {
      if (!strcmp(candidate->ln, safe_buf)) {
        __candidate_assignment(candidate, next, curr_index, is_inline_argument);
        option_active = xTRUE;
        break;
      }

    } else if (kind == XOPTIONS_KIND_POSITIONAL_OR_SUBCOMMAND) {
      if (candidate->subcommand &&
          !strcmp(candidate->subcommand->name, safe_buf)) {
        option_active = xTRUE;

        /* recursively parse subcommand */
        xoptions_parse(candidate->subcommand, argc - index, &argv[index]);

        /* stop parsing options at the current level, because parsing has
           entered a subcommand context. */
        xoptions_done(self, xFALSE, NULL);

        /* inherit error code from subcommand */
        self->err = candidate->subcommand->err;
        break;
      }
    }
  }

  /* if the current option is not active.
   *   - it is a positional argument.
   *   - it is an unrecognized option.
   */
  if (!option_active) {
    if (kind == XOPTIONS_KIND_POSITIONAL_OR_SUBCOMMAND) {
      XLOG_T("@%s, '%s' is a positional argument",
             __basename(self->first_argument),
             argv[index]);
      xlist_push_back(&self->positional_args, argv[index]);
    } else {
      xoptions_done(self,
                    xTRUE,
                    "error: unrecognized option '%s'\n\n",
                    argv[index]);
    }
  }

exit:
  if (heap_buf) {
    xbox_free(heap_buf);
  }
}

err_t xoptions_parse(xoptions self, int argc, char* argv[]) {
  if (!self || argc <= 0 || argv == NULL) return X_RET_INVAL;

  int idx = 0;
  self->first_argument = argv[idx++];

  if (self->prehook) self->prehook(self);

  while (idx < argc && self->done == xFALSE) {
    if (self->end_of_options == xTRUE) {
      XLOG_T("@%s, positional arg: '%s'",
             __basename(self->first_argument),
             argv[idx]);
      /* all of arguments after '--' are positional arguments */
      xlist_push_back(&self->positional_args, argv[idx]);
      ++idx;
    } else {
      /* try to match candidates */
      XLOG_T("@%s, parsing option: '%s'",
             __basename(self->first_argument),
             argv[idx]);
      __xoptions_try_match_candidate(self, &idx, argc, argv);
      ++idx;
    }
  }

  /*
   * Prevent running the required-option check in special cases,
   * such as when '-h' or '--help' is used. These options skip
   * normal validation flow.
   */
  if (self->done == xFALSE) {
    __xoptions_check_required(self);
  }

  if (self->posthook) self->posthook(self);

  return self->err;
}

void xoptions_done(xoptions self,
                   xbool_t print_help,
                   const char* error_fmt,
                   ...) {
  if (!self) return;

  self->done = xTRUE;

  if (error_fmt != NULL) {
    va_list args;
    va_start(args, error_fmt);
    vprintf(error_fmt, args);
    va_end(args);

    self->err = X_RET_ERROR;
  }

  if (error_fmt && print_help == xTRUE) {
    xoptions_helper_printf_advance(self, xFALSE, xFALSE, NULL);
  } else if (print_help == xTRUE) {
    xoptions_helper_printf_advance(self, xTRUE, xTRUE, NULL);
  }
}

static void __print_options(const xlist options) {
  if (!options || xlist_length(options) == 0) return;

  void* item = NULL;
  size_t max_len = 0;

  xlist_foreach(options, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    size_t candidate_len = strlen(candidate->ln) +
                           (candidate->sn != '\0' ? 1 : 0) +
                           strlen(candidate->hint);
    max_len = xMAX(max_len, candidate_len);
  }

  printf("Options:\n");
  xlist_foreach(options, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;

    __candidate_print(candidate, max_len, "  ", "  ");
  }

  printf("\n");
}

static void __print_commands(const xlist commands) {
  if (!commands || xlist_length(commands) == 0) return;

  void* item = NULL;
  size_t max_len = 0;

  xlist_foreach(commands, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    if (!candidate->subcommand) continue;

    size_t candidate_len = strlen(candidate->subcommand->name);
    max_len = xMAX(max_len, candidate_len);
  }

  printf("Commands:\n");
  xlist_foreach(commands, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    if (!candidate->subcommand) continue;

    printf("  %s  %s\n", candidate->subcommand->name, candidate->desc);
  }

  printf("\n");
}

void xoptions_helper_printf_advance(xoptions self,
                                    xbool_t whether_print_prefix,
                                    xbool_t whether_print_suffix,
                                    const char* prompt_fmt,
                                    ...) {
  if (!self) return;

  if (prompt_fmt) {
    va_list args;
    va_start(args, prompt_fmt);
    vprintf(prompt_fmt, args);
    va_end(args);
  }

  void* item;
  xlist options = xlist_create();
  xlist commands = xlist_create();

  assert(options && commands);

  xlist_foreach(&self->candidates, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    if (candidate->type == XOPTIONS_CANDIDATE_TYPE_SUBCOMMAND) {
      xlist_push_back(commands, candidate);
    } else {
      xlist_push_back(options, candidate);
    }
  }

  if (whether_print_prefix && strlen(self->prefix_prompt) > 0) {
    printf("%s\n", self->prefix_prompt);
  }

  printf("Usage:\n");
  printf("  %s %s %s [ARGS...]\n\n",
         __basename(self->first_argument),
         xlist_length(options) > 0 ? "[OPTIONS]" : "\b",
         xlist_length(commands) > 0 ? "COMMAND [COMMAND OPTIONS]" : "\b");

  if (strlen(self->desc) > 0) {
    printf("Description:\n");
    printf("  %s\n\n", self->desc);
  }

  __print_options(options);
  __print_commands(commands);

  if (whether_print_suffix && strlen(self->suffix_prompt) > 0) {
    printf("%s\n", self->suffix_prompt);
  }

  xlist_destroy(options);
  xlist_destroy(commands);
}

xlist xoptions_get_positional(xoptions self) {
  if (!self) return NULL;
  return &self->positional_args;
}

xoptions_candidate xoptions_add_string(xoptions self,
                                       char sn,
                                       const char* ln,
                                       const char* hint,
                                       const char* desc,
                                       char** ptr,
                                       xbool_t required) {
  xoptions_candidate c = __candidate_new();
  if (!c) return NULL;
  c->type = XOPTIONS_CANDIDATE_TYPE_STRING;
  c->sn = sn;
  c->ln = ln;
  c->hint = hint;
  c->desc = desc;
  c->storing_ptr = (void*)ptr;
  c->required = required;
  c->context = self;
  xlist_push_back(&self->candidates, c);
  return c;
}
xoptions_candidate xoptions_add_number(xoptions self,
                                       char sn,
                                       const char* ln,
                                       const char* hint,
                                       const char* desc,
                                       int* ptr,
                                       xbool_t required) {
  xoptions_candidate c = __candidate_new();
  if (!c) return NULL;
  c->type = XOPTIONS_CANDIDATE_TYPE_NUMBER;
  c->sn = sn;
  c->ln = ln;
  c->hint = hint;
  c->desc = desc;
  c->storing_ptr = (void*)ptr;
  c->required = required;
  c->context = self;
  xlist_push_back(&self->candidates, c);
  return c;
}
xoptions_candidate xoptions_add_boolean(
    xoptions self, char sn, const char* ln, const char* desc, xbool_t* ptr) {
  xoptions_candidate c = __candidate_new();
  if (!c) return NULL;
  c->type = XOPTIONS_CANDIDATE_TYPE_BOOLEAN;
  c->sn = sn;
  c->ln = ln;
  c->desc = desc;
  c->storing_ptr = (void*)ptr;
  c->required = xFALSE;
  c->context = self;
  xlist_push_back(&self->candidates, c);
  return c;
}
xoptions_candidate xoptions_add_action(xoptions self,
                                       char sn,
                                       const char* ln,
                                       const char* desc,
                                       xoptions_action_fn fn,
                                       void* user_data) {
  xoptions_candidate c = __candidate_new();
  if (!c) return NULL;
  c->type = XOPTIONS_CANDIDATE_TYPE_ACTION;
  c->sn = sn;
  c->ln = ln;
  c->desc = desc;
  c->fn = fn;
  c->storing_ptr = user_data;
  c->required = xFALSE;
  c->context = self;
  xlist_push_back(&self->candidates, c);
  return c;
}

xbool_t xoptions_candidate_is_used(xoptions_candidate self) {
  if (!self) return xFALSE;
  return self->used;
}

static err_t __atoi(const char* value, int* out) {
  long result = 0;
  int sign = 1;

  if (!value || !out) return X_RET_INVAL;

  if (*value == '+') {
    value++;
  } else if (*value == '-') {
    sign = -1;
    value++;
  }

  if (!isdigit((unsigned char)*value)) return X_RET_BADFMT;

  while (*value) {
    if (!isdigit((unsigned char)*value)) return X_RET_BADFMT;

    int digit = *value - '0';
    value++;

    if (sign > 0) {
      if (result > (LONG_MAX - digit) / 10) return X_RET_OVERFLOW;
    } else {
      if (-result < (LONG_MIN + digit) / 10) return X_RET_OVERFLOW;
    }

    result = result * 10 + digit;
  }

  result = result * sign;

  if (result > INT_MAX || result < INT_MIN) return X_RET_OVERFLOW;

  *out = (int)result;
  return X_RET_OK;
}

static err_t __candidate_assignment(xoptions_candidate self,
                                    const char* next,
                                    int* argv_index,
                                    xbool_t is_inline_arg) {
  self->used = xTRUE;

  if (self->type == XOPTIONS_CANDIDATE_TYPE_NUMBER)
    return __candidate_assignment_number(self, next, argv_index, is_inline_arg);
  else if (self->type == XOPTIONS_CANDIDATE_TYPE_BOOLEAN)
    return __candidate_assignment_boolean(self,
                                          next,
                                          argv_index,
                                          is_inline_arg);
  else if (self->type == XOPTIONS_CANDIDATE_TYPE_STRING)
    return __candidate_assignment_string(self, next, argv_index, is_inline_arg);
  else if (self->type == XOPTIONS_CANDIDATE_TYPE_ACTION)
    return __candidate_assignment_action(self, next);

  return X_RET_OK;
}

static err_t __candidate_assignment_number(xoptions_candidate self,
                                           const char* next,
                                           int* argv_index,
                                           xbool_t is_inline_arg) {
  err_t err = __atoi(next, (int*)self->storing_ptr);
  if (err != X_RET_OK) {
    xoptions_done(self->context,
                  xTRUE,
                  "error: invalid number value '%s' for option '[-%c | --%s]'"
                  "(%s)\n\n",
                  next ? next : "",
                  self->sn,
                  self->ln,
                  err_str(err));
  }

  // use next argument as value
  if (!is_inline_arg) (*argv_index)++;

  return err;
}
static err_t __candidate_assignment_boolean(xoptions_candidate self,
                                            const char* next,
                                            int* argv_index,
                                            xbool_t is_inline_arg) {
  if (next == NULL) {
    *(xbool_t*)self->storing_ptr = xTRUE;
  } else {
    if (strcasecmp(next, "true") == 0 || strcasecmp(next, "yes") == 0 ||
        strcasecmp(next, "on") == 0 || strcasecmp(next, "1") == 0) {
      *(xbool_t*)self->storing_ptr = xTRUE;

      // use next argument as value
      if (!is_inline_arg) (*argv_index)++;
    } else if (strcasecmp(next, "false") == 0 || strcasecmp(next, "no") == 0 ||
               strcasecmp(next, "off") == 0 || strcasecmp(next, "0") == 0) {
      *(xbool_t*)self->storing_ptr = xFALSE;

      // use next argument as value
      if (!is_inline_arg) (*argv_index)++;
    } else {
      *(xbool_t*)self->storing_ptr = xTRUE;
    }
  }
  return X_RET_OK;
}
static err_t __candidate_assignment_string(xoptions_candidate self,
                                           const char* value,
                                           int* argv_index,
                                           xbool_t is_inline_arg) {
  if (value == NULL) {
    xoptions_done(self->context,
                  xTRUE,
                  "error: missing value for option '[-%c | --%s]'\n\n",
                  self->sn,
                  self->ln);

    return X_RET_INVAL;
  } else {
    *(char**)self->storing_ptr = (char*)value;
    // use next argument as value
    if (!is_inline_arg) (*argv_index)++;
  }

  return X_RET_OK;
}
static err_t __candidate_assignment_action(xoptions_candidate self,
                                           const char* value) {
  self->fn(self->context, self->storing_ptr);
  return X_RET_OK;
}

static xbool_t __is_short_option(const char* name, char** start) {
  if (!name) return xFALSE;

  if (strlen(name) >= 2 && name[0] == '-' && name[1] != '-') {
    if (start) *start = (char*)(name + 1);
    return xTRUE;
  }

  return xFALSE;
}

static xbool_t __is_long_option(const char* name, char** start) {
  if (!name) return xFALSE;

  if (strlen(name) > 2 && name[0] == '-' && name[1] == '-') {
    if (start) *start = (char*)(name + 2);
    return xTRUE;
  }

  return xFALSE;
}

static xbool_t __is_positional_separator(const char* name) {
  if (!name) return xFALSE;

  if (!strcmp(name, "--")) {
    return xTRUE;
  }

  return xFALSE;
}

static xoptions_candidate __candidate_new() {
  xoptions_candidate self = (xoptions_candidate)xbox_malloc(
      sizeof(struct xoptions_candidate_priv));
  if (!self) return NULL;

  self->type = XOPTIONS_CANDIDATE_TYPE_END;
  self->sn = '\0';
  self->ln = "";
  self->hint = "";
  self->desc = "";
  self->subcommand = NULL;
  self->fn = NULL;
  self->storing_ptr = NULL;
  self->required = xFALSE;
  self->used = xFALSE;
  self->context = NULL;
  return self;
}

static void __candidate_free(xoptions_candidate self) {
  if (!self) return;

  if (self->subcommand) xoptions_destroy(self->subcommand);

  xbox_free(self);
}

static void __candidate_print(xoptions_candidate self,
                              size_t left_width,
                              const char* front_padding,
                              const char* back_padding) {
  char buf[128];
  char* pos = buf;
  size_t offset = 0;

  strcpy(pos, front_padding);
  pos += strlen(front_padding);

  if (self->sn != '\0') {
    pos[offset++] = '-';
    pos[offset++] = self->sn;
    pos[offset++] = ',';
    pos[offset++] = ' ';
  }

  if (strlen(self->ln) > 0) {
    pos[offset++] = '-';
    pos[offset++] = '-';
    strcpy(&pos[offset], self->ln);
    offset += strlen(self->ln);
  }

  if (strlen(self->hint) > 0) {
    pos[offset++] = ' ';
    strcpy(&pos[offset], self->hint);
    offset += strlen(self->hint);
  }

  if (offset < left_width) {
    memset(&pos[offset], ' ', left_width + strlen("---, ") - offset);
    offset = left_width + strlen("---, ");
  }

  strcpy(&pos[offset], back_padding);
  pos += strlen(back_padding);

  if (strlen(self->desc) > 0) {
    pos[offset++] = ' ';
    strcpy(&pos[offset], self->desc);
  }

  printf("%s\n", buf);
}

static void __xoptions_default_helper_action(xoptions self, void* user_data) {
  xoptions_done(self, xTRUE, NULL);
  xbox_exit(0);
}

static void __xoptions_check_required(xoptions self) {
  if (!self) return;

  void* item;
  xlist_foreach(&self->candidates, item) {
    xoptions_candidate candidate = (xoptions_candidate)item;
    if (candidate->required == xTRUE && candidate->used == xFALSE) {
      if (candidate->sn != '\0' && strlen(candidate->ln) > 0) {
        xoptions_done(self,
                      xTRUE,
                      "error: missing required option '-%c' / '--%s'\n\n",
                      candidate->sn,
                      candidate->ln);
      } else if (strlen(candidate->ln) > 0) {
        xoptions_done(self,
                      xTRUE,
                      "error: missing required option '--%s'\n\n",
                      candidate->ln);
      } else if (candidate->sn != '\0') {
        xoptions_done(self,
                      xTRUE,
                      "error: missing required option '-%c'\n\n",
                      candidate->sn);
      }
    }
  }
}

static const char* __basename(const char* path) {
  if (path == NULL) return "";

  const char* last_slash = strrchr(path, '/');
  return last_slash ? last_slash + 1 : path;
}
