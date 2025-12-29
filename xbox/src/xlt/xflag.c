/**
 * @brief 结构化命令行标志解析
 * @file xflag.c
 * @author Oswin
 * @date 2025-06-26
 * @details A simple and lightweight command-line flag parsing library.
 * It allows defining flags of different types (integer, boolean, string) and
 * parsing them from `argv`.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "xflag.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "xlist.h"

struct xflag_priv {
  const char* prog;                   /**< the program name */
  char error[128];                    /**< the last error message */
  xlist args;                         /**< the list of arguments */
  xbool_t enable_default_description; /**< enable default description */
  xbool_t parse_exit;                 /**< enable exit on parse error */
};

typedef enum {
  XFLAG_INT = 0,
  XFLAG_STRING,
  XFLAG_BOOL,
  XFLAG_HANDLER,
} xflag_type;

typedef struct {
  xflag_type type;         /**< the argument type */
  char* short_name;        /**< the short name */
  char* long_name;         /**< the long name */
  const char* description; /**< the argument description */
  union {
    int* integer;     /**< Pointer to an int variable (for integer arguments) */
    char** string;    /**< Pointer to a string array (for string arguments) */
    xbool_t* boolean; /**< Pointer to a boolean variable (for flag arguments) */
    struct {
      xflag_handler
          action;      /**< Callback handler function for custom handling */
      void* user_data; /**< User-provided data passed to the handler */
    } value_ptr;       /**< Handler structure for custom argument processing */
  };
} xflag_arg;

static xflag_arg* make_arg(const char* short_name,
                           const char* long_name,
                           const char* description,
                           xflag_type t,
                           void* v);
static xflag_arg* make_handler_arg(const char* short_name,
                                   const char* long_name,
                                   const char* description,
                                   xflag_handler f,
                                   void* d);
static xbool_t compare_arg(const xflag_arg* a, const xflag_arg* b);
static err_t parse_arg(xflag_arg* arg,
                       const char* cur,
                       const char* next,
                       xbool_t* skip_next,
                       xflag context);
static void free_arg(void* arg);
static const char* arg_default_value_string(xflag_arg* arg);

static void xflag_set_error(xflag self, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(self->error, sizeof(self->error), fmt, ap);
  va_end(ap);
}

static void xflag_append_arg(xflag f, xflag_arg* arg) {
  if (f == NULL || arg == NULL) return;

  xlist_push_back(f->args, arg);
}

static void xflag_print_help(void* param) {
  xflag f = param;
  if (!f) return;

  printf("\nUsage: %s [options]\n\n", f->prog);

  void* data_ptr;
  int max_len = 0;
  xlist_foreach(f->args, data_ptr) {
    xflag_arg* arg = (xflag_arg*)data_ptr;
    int len = 0;
    if (arg->short_name && arg->long_name) {
      len = strlen(arg->short_name) + 2 + strlen(arg->long_name);  // "s, l"
    } else if (arg->short_name) {
      len = strlen(arg->short_name);
    } else if (arg->long_name) {
      len = strlen(arg->long_name);
    }
    if (len > max_len) max_len = len;
  }

  xlist_foreach(f->args, data_ptr) {
    xflag_arg* arg = (xflag_arg*)data_ptr;
    char buf[128];
    if (arg->short_name && arg->long_name) {
      snprintf(buf, sizeof(buf), "%s, %s", arg->short_name, arg->long_name);
    } else if (arg->short_name) {
      snprintf(buf, sizeof(buf), "%s", arg->short_name);
    } else if (arg->long_name) {
      snprintf(buf, sizeof(buf), "%s", arg->long_name);
    }

    printf("  %-*s %s %s\n",
           max_len,
           buf,
           arg->description,
           f->enable_default_description ? arg_default_value_string(arg) : "");
  }

  printf("\n");

#if defined(__unix__) || defined(__MACH__)
  /*
   * In Linux or macOS
   * helper will exit application.
   */
  xbox_exit(0);
#else
  /* In RT-Thread or FreeRTOS or other RTOS
   * CLI is a thread/task, so we could not exit application here.
   * use `parse_exit` flag to exit `xflag_parse` function.
   * */
  f->parse_exit = xTRUE;
#endif
}

xflag xflag_create() {
  xflag f = (xflag)xbox_malloc(sizeof(struct xflag_priv));
  if (!f) return NULL;
  memset(f, 0, sizeof(struct xflag_priv));

  f->args = xlist_create();
  if (!f->args) {
    xbox_free(f);
    return NULL;
  }

  /* built-in helper */
  xflag_add_handler(f,
                    "h",
                    "help",
                    "Display available options",
                    xflag_print_help,
                    f);

  return f;
}

err_t xflag_add_int(xflag self,
                    const char* short_name,
                    const char* long_name,
                    const char* description,
                    int* value) {
  if (!self) return X_RET_INVAL;

  xflag_arg* arg = make_arg(short_name,
                            long_name,
                            description,
                            XFLAG_INT,
                            value);
  if (!arg) return X_RET_ERROR;

  xflag_append_arg(self, arg);

  return X_RET_OK;
}

err_t xflag_add_bool(xflag self,
                     const char* short_name,
                     const char* long_name,
                     const char* description,
                     xbool_t* value) {
  if (!self) return X_RET_INVAL;

  xflag_arg* arg = make_arg(short_name,
                            long_name,
                            description,
                            XFLAG_BOOL,
                            value);
  if (!arg) return X_RET_ERROR;

  xflag_append_arg(self, arg);

  return X_RET_OK;
}

err_t xflag_add_string(xflag self,
                       const char* short_name,
                       const char* long_name,
                       const char* description,
                       char** value) {
  if (!self) return X_RET_INVAL;

  xflag_arg* arg = make_arg(short_name,
                            long_name,
                            description,
                            XFLAG_STRING,
                            value);
  if (!arg) return X_RET_ERROR;

  xflag_append_arg(self, arg);

  return X_RET_OK;
}

err_t xflag_add_handler(xflag self,
                        const char* short_name,
                        const char* long_name,
                        const char* description,
                        xflag_handler f,
                        void* d) {
  if (!self) return X_RET_INVAL;

  xflag_arg* arg = make_handler_arg(short_name, long_name, description, f, d);
  if (!arg) return X_RET_ERROR;

  xflag_append_arg(self, arg);

  return X_RET_OK;
}

err_t xflag_enable_default_description(xflag self) {
  if (!self) return X_RET_INVAL;

  self->enable_default_description = xTRUE;

  return X_RET_OK;
}

err_t xflag_set_program_name(xflag self, const char* name) {
  if (!self || !name) return X_RET_INVAL;

  self->prog = name;

  return X_RET_OK;
}

err_t xflag_parse(xflag self, int argc, char** argv) {
  assert(argc > 0);
  assert(argv != NULL);

  if (self == NULL) return X_RET_INVAL;

  if (self->prog == NULL) {
    self->prog = strrchr(argv[0], '/') + 1;
    if (self->prog == (void*)0x1) self->prog = argv[0];
  }

  char** pos = argv + 1;
  while (*pos) {
    char* cur = *pos;
    char* next = *(pos + 1);
    xbool_t ok = xFALSE;
    xbool_t skip_next = xFALSE;
    xflag_arg key = {0};
    if (cur[0] == '-') {
      if (cur[1] == '-') {
        key.long_name = cur;
      } else {
        key.short_name = cur;
      }
    }

    void* data_ptr;
    xlist_foreach(self->args, data_ptr) {
      xflag_arg* arg = (xflag_arg*)data_ptr;
      if (compare_arg(&key, arg) == xTRUE) {
        err_t err = parse_arg(arg, cur, next, &skip_next, self);
        if (err != X_RET_OK) {
          printf("%s\n", xflag_error(self));
          xflag_print_help(self);
          return err;
        }

        ok = xTRUE;
        break;
      }
    }

    if (!ok) {
      printf("flag provided but not defined: \"%s\"\n\n", cur);
      xflag_print_help(self);
      return X_RET_ERROR;
    }

    if (skip_next) {
      pos += 2;
    } else {
      pos++;
    }

    if (self->parse_exit == xTRUE) break;
  }

  return X_RET_OK;
}

err_t xflag_destroy(xflag self) {
  if (self == NULL) return X_RET_INVAL;

  xlist_drain(self->args, free_arg);
  xlist_destroy(self->args);

  xbox_free(self);

  return X_RET_OK;
}

const char* xflag_error(xflag self) {
  if (self == NULL) return "";

  return self->error;
}

static xbool_t compare_arg(const xflag_arg* a, const xflag_arg* b) {
  if (!a || !b) return xFALSE;

  if (a->short_name && b->short_name)
    return !strcmp(a->short_name, b->short_name);

  if (a->long_name && b->long_name) return !strcmp(a->long_name, b->long_name);

  return xFALSE;
}

static xflag_arg* make_arg(const char* short_name,
                           const char* long_name,
                           const char* description,
                           xflag_type t,
                           void* v) {
  if ((!short_name || strlen(short_name) == 0) && !long_name &&
      strlen(description) == 0)
    return NULL;

  xflag_arg* arg = (xflag_arg*)xbox_malloc(sizeof(xflag_arg));
  arg->type = t;
  arg->short_name = NULL;
  arg->long_name = NULL;
  arg->description = description;

  if (short_name && strlen(short_name)) {
    xbox_asprintf(&arg->short_name, "-%s", short_name);
  }
  if (long_name && strlen(long_name)) {
    xbox_asprintf(&arg->long_name, "--%s", long_name);
  }

  switch (t) {
    case XFLAG_INT:
      arg->integer = (int*)v;
      break;
    case XFLAG_STRING:
      arg->string = (char**)v;
      break;
    case XFLAG_BOOL:
      arg->boolean = (xbool_t*)v;
      break;
    case XFLAG_HANDLER:
      arg->value_ptr.action = (xflag_handler)v;
      arg->value_ptr.user_data = NULL;
      break;
  }

  return arg;
}

static xflag_arg* make_handler_arg(const char* short_name,
                                   const char* long_name,
                                   const char* description,
                                   xflag_handler f,
                                   void* d) {
  xflag_arg* arg = make_arg(short_name,
                            long_name,
                            description,
                            XFLAG_HANDLER,
                            f);
  if (arg == NULL) return NULL;

  arg->value_ptr.user_data = d;
  return arg;
}

static const char* arg_default_value_string(xflag_arg* arg) {
  assert(arg);

  static char buf[64] = {0};

  switch (arg->type) {
    case XFLAG_INT: {
      snprintf(buf, sizeof(buf), "(default: %d)", *arg->integer);
      return buf;
    }
    case XFLAG_STRING: {
      if (*arg->string) {
        snprintf(buf, sizeof(buf), "(default: %s)", *arg->string);
        return buf;
      } else
        return "";
    }
    case XFLAG_BOOL: {
      if (*arg->boolean)
        snprintf(buf, sizeof(buf), "(default: true)");
      else
        snprintf(buf, sizeof(buf), "(default: false)");
      return buf;
    }
    case XFLAG_HANDLER:
    default:
      return "";
  }
}

static void free_arg(void* p) {
  xflag_arg* arg = (xflag_arg*)p;
  if (!arg) return;

  if (arg->long_name) xbox_free(arg->long_name);

  if (arg->short_name) xbox_free(arg->short_name);

  xbox_free(arg);
}

static err_t parse_arg(xflag_arg* arg,
                       const char* cur,
                       const char* next,
                       xbool_t* skip_next,
                       xflag context) {
  if (!arg || !context) return X_RET_INVAL;

  err_t err = X_RET_OK;
  *skip_next = xFALSE;

  if (arg->type == XFLAG_INT) {
    char* endptr = NULL;
    if (next == NULL) {
      xflag_set_error(context, "flag \"%s\" needs an integer argument", cur);
      return X_RET_ERROR;
    }

    strtol(next, &endptr, 10);
    if (*endptr != '\0' || endptr == next) {
      xflag_set_error(context,
                      "flag \"%s\" needs an integer argument, but \"%s\" is "
                      "not an integer",
                      cur,
                      next);
      return X_RET_ERROR;
    }

    *arg->integer = atoi(next);

    *skip_next = xTRUE;

  } else if (arg->type == XFLAG_STRING) {
    if (next == NULL) {
      xflag_set_error(context, "flag \"%s\" needs a string argument", cur);
      return X_RET_ERROR;
    }

    *arg->string = (char*)next;

    *skip_next = xTRUE;

  } else if (arg->type == XFLAG_BOOL) {
    if (next == NULL || next[0] == '-') {
      *arg->boolean = xTRUE;
    } else {
      if (!strcasecmp(next, "false") || !strcasecmp(next, "no") ||
          !strcasecmp(next, "off") || !strcmp(next, "0") ||
          !strcmp(next, "n")) {
        *arg->boolean = xFALSE;
        *skip_next = xTRUE;
      } else if (!strcasecmp(next, "true") || !strcasecmp(next, "yes") ||
                 !strcasecmp(next, "on") || !strcmp(next, "1") ||
                 !strcmp(next, "y")) {
        *arg->boolean = xTRUE;
        *skip_next = xTRUE;
      } else {
        xflag_set_error(context,
                        "flag \"%s\" needs a boolean argument, but \"%s\" is "
                        "not a built-in boolean",
                        cur,
                        next);
        return X_RET_ERROR;
      }
    }
  } else if (arg->type == XFLAG_HANDLER) {
    arg->value_ptr.action(arg->value_ptr.user_data);
  }

  return err;
}
