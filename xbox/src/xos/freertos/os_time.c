/**
 * @brief FreeRTOS 时间
 * @file os_time.c
 * @author Oswin
 * @date 2025-06-25
 * @details This file provides the time implementation for FreeRTOS.
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

os_tick_t os_time_now(void) { return (os_tick_t)time(NULL); }

os_tick_t os_time_now_millis(void) {
  TickType_t ticks = xTaskGetTickCount();
  return ((os_tick_t)ticks * 1000) / configTICK_RATE_HZ;
}

os_tick_t os_time_now_micros(void) {
  TickType_t ticks = xTaskGetTickCount();
  return ((os_tick_t)ticks * 1000000) / configTICK_RATE_HZ;
}

os_tick_t os_time_now_nanos(void) {
  TickType_t ticks = xTaskGetTickCount();
  return ((os_tick_t)ticks * 1000000000) / configTICK_RATE_HZ;
}

err_t os_time_string(char* buf, size_t len) {
  return os_time_string_spec(time(NULL), buf, len);
}

err_t os_time_string_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* "2025-07-09 19:46:23" */
  if (len < 20) return X_RET_ERROR;

  struct tm tm_time;
  localtime_r(&t, &tm_time);

  snprintf(buf,
           len,
           "%04d-%02d-%02d %02d:%02d:%02d",
           tm_time.tm_year + 1900,
           tm_time.tm_mon + 1,
           tm_time.tm_mday,
           tm_time.tm_hour,
           tm_time.tm_min,
           tm_time.tm_sec);

  return X_RET_OK;
}

err_t os_time_string2(char* buf, size_t len) {
  return os_time_string2_spec(time(NULL), buf, len);
}

err_t os_time_string2_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 20250709_194623 */
  if (len < 16) return X_RET_INVAL;

  struct tm tm_time;
  localtime_r(&t, &tm_time);
  snprintf(buf,
           len,
           "%04d%02d%02d_%02d%02d%02d",
           tm_time.tm_year + 1900,
           tm_time.tm_mon + 1,
           tm_time.tm_mday,
           tm_time.tm_hour,
           tm_time.tm_min,
           tm_time.tm_sec);

  return X_RET_OK;
}

err_t os_time_string3(char* buf, size_t len) {
  return os_time_string3_spec(time(NULL), buf, len);
}

err_t os_time_string3_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 2025/07/09 19:46:23 */
  if (len < 20) return X_RET_INVAL;

  struct tm tm_time;
  localtime_r(&t, &tm_time);
  snprintf(buf,
           len,
           "%04d/%02d/%02d %02d:%02d:%02d",
           tm_time.tm_year + 1900,
           tm_time.tm_mon + 1,
           tm_time.tm_mday,
           tm_time.tm_hour,
           tm_time.tm_min,
           tm_time.tm_sec);

  return X_RET_OK;
}

err_t os_time_string_with_ms(char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 2025-07-09 19:46:23.123 */
  if (len < 24) return X_RET_INVAL;

  os_tick_t now_ms = os_time_now_millis();
  time_t now_sec = now_ms / 1000;
  int ms = now_ms % 1000;

  struct tm tm_time;
  localtime_r(&now_sec, &tm_time);

  int ret = snprintf(buf,
                     len,
                     "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                     tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1,
                     tm_time.tm_mday,
                     tm_time.tm_hour,
                     tm_time.tm_min,
                     tm_time.tm_sec,
                     ms);

  if (ret < 0 || (size_t)ret >= len) return X_RET_INVAL;

  return X_RET_OK;
}

err_t os_time_string_iso8601(char* buf, size_t len) {
  return os_time_string_iso8601_spec(time(NULL), buf, len);
}

err_t os_time_string_iso8601_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len < 26) return X_RET_INVAL;

  struct tm local_tm;
  if (localtime_r(&t, &local_tm) == NULL) return X_RET_ERROR;

  // TODO: FreeRTOS does not have a standard way to get timezone offset.
  // tm_gmtoff is a non-standard extension. A platform-specific implementation
  // is required to get the correct timezone offset.
  int offset_hours = 0;
  int offset_minutes = 0;
  char sign = '+';

  int n = snprintf(buf,
                   len,
                   "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
                   local_tm.tm_year + 1900,
                   local_tm.tm_mon + 1,
                   local_tm.tm_mday,
                   local_tm.tm_hour,
                   local_tm.tm_min,
                   local_tm.tm_sec,
                   sign,
                   abs(offset_hours),
                   offset_minutes);

  if (n != 25) return X_RET_ERROR;

  return X_RET_OK;
}

err_t os_time_string_rfc2822(char* buf, size_t len) {
  return os_time_string_rfc2822_spec(time(NULL), buf, len);
}

err_t os_time_string_rfc2822_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len < 32) return X_RET_INVAL;

  struct tm tm_time;
  localtime_r(&t, &tm_time);
  // The output of %Z is implementation-defined. For GMT/UTC, it might be "GMT".
  // For other timezones, it depends on the C library and OS configuration.
  size_t n = strftime(buf, len, "%a, %d %b %Y %H:%M:%S GMT", &tm_time);

  return n > 0 ? X_RET_OK : X_RET_ERROR;
}
