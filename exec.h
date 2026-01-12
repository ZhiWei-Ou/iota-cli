/**
 * @brief exec header
 * @file exec.h
 * @author Oswin
 * @date 2025-12-26
 * @details
 */
#ifndef EXEC_H_
#define EXEC_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xstring.h"

typedef struct {
    int code;
    xstring output;
} exec_t;

exec_t exec_command(const char *command);
#define exec_code(exe) ((exe).code)
#define exec_output(exe) xstring_to_string(&(exe).output)
#define exec_success(exe) ((exe).code == 0)
#define exec_free(exe) do { \
    xstring_free(&(exe).output); \
} while (0)


void assert_command(const char *cmd);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* EXEC_H_ */
