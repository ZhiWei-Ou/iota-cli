/**
 * @brief RT-Thread 时间
 * @file os_time.c
 * @author Oswin
 * @date 2025-06-25
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
#include "os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

os_tick_t os_time_now(void) { return (os_tick_t)time(NULL); }

os_tick_t os_time_now_millis(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (os_tick_t)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

os_tick_t os_time_now_micros(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (os_tick_t)tv.tv_sec * 1000000LL + tv.tv_usec;
}

os_tick_t os_time_now_nanos(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (os_tick_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

err_t os_time_string(char* buf, size_t len) {
  return os_time_string_spec(time(NULL), buf, len);
}

err_t os_time_string_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* char buffer[20] = "2025-07-09 19:46:23"; */
  if (len < 20) return X_RET_ERROR;

  struct tm* tm = localtime(&t);
  snprintf(buf,
           len,
           "%04d-%02d-%02d %02d:%02d:%02d",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec);

  return X_RET_OK;
}

err_t os_time_string2(char* buf, size_t len) {
  return os_time_string2_spec(time(NULL), buf, len);
}

err_t os_time_string2_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 20250709_194623 */
  if (len < 16) return X_RET_INVAL;

  struct tm* tm = localtime(&t);
  snprintf(buf,
           len,
           "%04d%02d%02d_%02d%02d%02d",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec);

  return X_RET_OK;
}

err_t os_time_string3(char* buf, size_t len) {
  return os_time_string3_spec(time(NULL), buf, len);
}

err_t os_time_string3_spec(time_t t, char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 2025/07/09 19:46:23 */
  if (len < 20) return X_RET_INVAL;

  struct tm* tm = localtime(&t);
  snprintf(buf,
           len,
           "%04d/%02d/%02d %02d:%02d:%02d",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec);

  return X_RET_OK;
}

err_t os_time_string_with_ms(char* buf, size_t len) {
  if (buf == NULL || len == 0) return X_RET_INVAL;

  /* 2025-07-09 19:46:23.123 */
  if (buf == NULL || len == 0) return X_RET_INVAL;

  if (len < 24) return X_RET_INVAL;

  struct timeval tv;
  gettimeofday(&tv, NULL);

  time_t t = tv.tv_sec;
  struct tm tm_time;
  localtime_r(&t, &tm_time);

  int ms = tv.tv_usec / 1000;

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

  long gmtoff_sec = 0;

#if defined(__APPLE__) || defined(__linux__)
  gmtoff_sec = local_tm.tm_gmtoff;
#else
  return X_RET_ERROR;  // 简化处理
#endif

  int offset_hours = gmtoff_sec / 3600;
  int offset_minutes = (abs((int)gmtoff_sec) % 3600) / 60;

  char sign = (gmtoff_sec >= 0) ? '+' : '-';

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

  struct tm* lt = localtime(&t);
  size_t n = strftime(buf, len, "%a, %d %b %Y %H:%M:%S %Z", lt);

  return n > 0 ? X_RET_OK : X_RET_ERROR;
}
