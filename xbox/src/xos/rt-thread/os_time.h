/**
 * @brief RT-Thread 时间
 * @file os_time.h
 * @author Oswin
 * @date 2025-06-25
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#ifndef OS_TIME__H_
#define OS_TIME__H_

#include <time.h>

#include "xdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef long long os_tick_t;

/* second-level tick */
os_tick_t os_time_now(void);
/* millisecond-level tick */
os_tick_t os_time_now_millis(void);
/* microsecond-level tick */
os_tick_t os_time_now_micros(void);
/* nanosecond-level tick */
os_tick_t os_time_now_nanos(void);

/* 2025-07-09 19:46:23 */
err_t os_time_string(char* buf, size_t len);
err_t os_time_string_spec(time_t t, char* buf, size_t len);

/* 2025-07-09 19:46:23.123 */
err_t os_time_string_with_ms(char* buf, size_t len);

/* 20250709_194623 */
err_t os_time_string2(char* buf, size_t len);
err_t os_time_string2_spec(time_t t, char* buf, size_t len);

/* 2025/07/09 19:46:23 */
err_t os_time_string3(char* buf, size_t len);
err_t os_time_string3_spec(time_t t, char* buf, size_t len);

/* 2025-07-09T11:46:23+08:00 */
err_t os_time_string_iso8601(char* buf, size_t len);
err_t os_time_string_iso8601_spec(time_t t, char* buf, size_t len);

/* RFC 2822 / RFC 1123 */
/* Wed, 09 Jul 2025 19:46:23 GMT */
err_t os_time_string_rfc2822(char* buf, size_t len);
err_t os_time_string_rfc2822_spec(time_t t, char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* OS_TIME__H_ */
