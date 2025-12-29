/**
 * @brief Unix 时间
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

/** @brief Represents a tick value, typically in a high-resolution format. */
typedef long long os_tick_t;

/**
 * @brief Gets the current time in seconds.
 * @return The current time in seconds.
 */
os_tick_t os_time_now(void);

/**
 * @brief Gets the current time in milliseconds.
 * @return The current time in milliseconds.
 */
os_tick_t os_time_now_millis(void);

/**
 * @brief Gets the current time in microseconds.
 * @return The current time in microseconds.
 */
os_tick_t os_time_now_micros(void);

/**
 * @brief Gets the current time in nanoseconds.
 * @return The current time in nanoseconds.
 */
os_tick_t os_time_now_nanos(void);

/**
 * @brief Formats the current time as a string (e.g., "2025-07-09 19:46:23").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string(char* buf, size_t len);

/**
 * @brief Formats a specific time as a string (e.g., "2025-07-09 19:46:23").
 * @param t The time_t value to format.
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_spec(time_t t, char* buf, size_t len);

/**
 * @brief Formats the current time as a string with milliseconds (e.g., "2025-07-09 19:46:23.123").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_with_ms(char* buf, size_t len);

/**
 * @brief Formats the current time as a string (e.g., "20250709_194623").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string2(char* buf, size_t len);

/**
 * @brief Formats a specific time as a string (e.g., "20250709_194623").
 * @param t The time_t value to format.
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string2_spec(time_t t, char* buf, size_t len);

/**
 * @brief Formats the current time as a string (e.g., "2025/07/09 19:46:23").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string3(char* buf, size_t len);

/**
 * @brief Formats a specific time as a string (e.g., "2025/07/09 19:46:23").
 * @param t The time_t value to format.
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string3_spec(time_t t, char* buf, size_t len);

/**
 * @brief Formats the current time as an ISO 8601 string (e.g., "2025-07-09T11:46:23+08:00").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_iso8601(char* buf, size_t len);

/**
 * @brief Formats a specific time as an ISO 8601 string (e.g., "2025-07-09T11:46:23+08:00").
 * @param t The time_t value to format.
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_iso8601_spec(time_t t, char* buf, size_t len);

/**
 * @brief Formats the current time as an RFC 2822 / RFC 1123 string (e.g., "Wed, 09 Jul 2025 19:46:23 GMT").
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_rfc2822(char* buf, size_t len);

/**
 * @brief Formats a specific time as an RFC 2822 / RFC 1123 string (e.g., "Wed, 09 Jul 2025 19:46:23 GMT").
 * @param t The time_t value to format.
 * @param buf The buffer to store the formatted string.
 * @param len The size of the buffer.
 * @return XBOX_OK on success, or an error code on failure.
 */
err_t os_time_string_rfc2822_spec(time_t t, char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* OS_TIME__H_ */
