/**
 * @brief 字符串
 * @file xstring.c
 * @author Oswin
 * @date 2025-12-06
 * @details
 *
 * @copyright (c) 2025 Intretech Software Development Department. All Rights Reserved.
 */
// #define XBOX_ENABLE_BACKTRACE
#include "xstring.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* xstring_double_expand_with_minimum(xstring* s, size_t min) {
  const char* h = s->s;
  s->s = xbox_realloc(s->s, xMAX(s->cap * 2, min));
  s->cap = xMAX(s->cap * 2, min);

  if (!h) strcpy(s->s, s->b);

  return s->s;
}

xstring xstring_init_iter(const char* str) {
  xstring s = xstring_init_empty();
  if (!str) {
    return s;
  }

  char* p = s.b;

  if ((strlen(str) + 1) >= s.cap) {
    s.s = xbox_malloc(strlen(str) + 1);
    if (!s.s) {
      return s;
    }

    p = s.s;
    s.cap = strlen(str) + 1;
  }

  s.len = strlen(str);
  strcpy(p, str);

  return s;
}

void xstring_init_iter_r(xstring* s, const char* str) {
  if (!s || !str) {
    return;
  }

  if ((strlen(str) + 1) > s->cap) {
    xstring_double_expand_with_minimum(s, strlen(str) + 1);
    strcpy(s->s, str);
  } else {
    char* p = s->s ? s->s : s->b;
    strcpy(p, str);
  }

  s->len = strlen(str);
}

xstring xstring_init_from_other(const xstring* other) {
  xstring s = xstring_init_empty();
  if (!other) {
    return s;
  }

  s.len = other->len;
  s.cap = other->cap;
  if (s.s) {
    strcpy(s.s, other->s);
  } else {
    strcpy(s.b, other->b);
  }

  return s;
}

void xstring_init_from_other_r(xstring* s1, const xstring* s2) {
  if (!s1 || !s2) {
    return;
  }

  xstring_init_iter_r(s1, xstring_to_string(s2));
}

xstring xstring_init_format(const char* fmt, ...) {
  if (!fmt) {
    return xstring_init_empty();
  }

  va_list ap, ap_copy;
  va_start(ap, fmt);

  return xstring_init_vformat(fmt, ap);
}

xstring xstring_init_vformat(const char* fmt, va_list ap) {
  xstring s = xstring_init_empty();
  if (!fmt) {
    return s;
  }

  xstring_init_vformat_r(&s, fmt, ap);
  return s;
}

void xstring_init_format_r(xstring* s, const char* fmt, ...) {
  if (!s || !fmt) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  xstring_init_vformat_r(s, fmt, ap);
  va_end(ap);
}

void xstring_init_vformat_r(xstring* s, const char* fmt, va_list ap) {
  if (!s || !fmt) {
    return;
  }

  char* p = s->b;

  va_list ap_copy;
  va_copy(ap_copy, ap);
  int len = vsnprintf(NULL, 0, fmt, ap_copy);
  va_end(ap_copy);
  if (len < 0) {
    return;
  }

  if ((len + 1) > s->cap) {
    s->s = xbox_malloc(len + 1);
    if (!s->s) {
      return;
    }

    p = s->s;
    s->cap = len + 1;
  }

  s->len = vsnprintf(p, s->cap, fmt, ap);
}

void xstring_free(xstring* s) {
  if (!s) return;

  if (s->s) xbox_free(s->s);
  memset(s, 0, sizeof(xstring));
}

char xstring_at(const xstring* s, int index) {
  if (!s) return 0;
  const char* p = s->s ? s->s : s->b;

  if (index < 0 || index >= s->len) return 0;
  return p[index];
}

const char* xstring_start(const xstring* s, int index) {
  if (!s) return NULL;
  const char* p = s->s ? s->s : s->b;
  if (index < 0 || index > s->len) return NULL;
  return p + index;
}

const char* xstring_to_string(const xstring* s) {
  if (!s) return NULL;
  return s->s ? s->s : s->b;
}

int xstring_length(const xstring* s) {
  if (!s) return 0;
  return s->len;
}

int xstring_capcity(const xstring* s) {
  if (!s) return 0;
  return s->cap;
}

void xstring_clear(xstring* s) {
  if (!s) return;
  s->len = 0;
  memset(s->b, 0, xARRAY_SIZE(s->b));

  if (s->s) {
    xbox_free(s->s);
    s->s = NULL;
  }
}

const char* xstring_trim(xstring* s) {
  if (!s || s->len == 0) return xstring_to_string(s);

  char* p = s->s ? s->s : s->b;
  const char* start = p;

  // find the first non-whitespace character
  while (*start && isspace((unsigned char)*start)) {
    start++;
  }

  // if the string is all whitespace
  if (*start == '\0') {
    s->len = 0;
    p[0] = '\0';
    return p;
  }

  // find the last non-whitespace character
  char* end = p + s->len - 1;
  while (end > start && isspace((unsigned char)*end)) {
    end--;
  }

  s->len = end - start + 1;
  // Use memmove because src and dest may overlap
  memmove(p, start, s->len);
  p[s->len] = '\0';

  return p;
}

const char* xstring_trim_left(xstring* s) {
  if (!s || s->len == 0) return xstring_to_string(s);

  char* p = s->s ? s->s : s->b;
  const char* start = p;

  while (*start && isspace((unsigned char)*start)) {
    start++;
  }

  s->len = s->len - (start - p);
  memmove(p, start, s->len);
  p[s->len] = '\0';

  return p;
}

const char* xstring_trim_right(xstring* s) {
  if (!s || s->len == 0) return xstring_to_string(s);

  char* p = s->s ? s->s : s->b;
  char* end = p + s->len - 1;

  while (end >= p && isspace((unsigned char)*end)) {
    end--;
  }

  s->len = end < p ? 0 : end - p + 1;
  p[s->len] = '\0';

  return p;
}

xbool_t xstring_is_empty(const xstring* s) {
  if (!s) return xTRUE;
  return s->len == 0;
}

const char* xstring_cat(xstring* s, const char* str) {
  if (!s) return "";

  if (!str) return xstring_to_string(s);

  size_t len = strlen(str);
  if (len == 0) return xstring_to_string(s);

  if ((len + s->len) >= s->cap) {
    xstring_double_expand_with_minimum(s, s->len + len + 1);
  }

  char* p = s->s ? s->s : s->b;
  memcpy(p + s->len, str, len + 1);
  s->len += len;
  return p;
}

const char* xstring_prepend(xstring* s, const char* str) {
  if (!s) return "";

  if (!str) return xstring_to_string(s);

  size_t len = strlen(str);
  if (len == 0) return xstring_to_string(s);

  if ((len + s->len) >= s->cap) {
    xstring_double_expand_with_minimum(s, s->len + len + 1);
  }

  char* p = s->s ? s->s : s->b;
  memmove(p + len, p, s->len + 1);
  memcpy(p, str, len);
  s->len += len;
  return p;
}

const char* xstring_upper(const xstring* s) {
  if (!s) return "";
  const char* p = s->s ? s->s : s->b;
  char* q = (char*)p;
  while (*q) *q = toupper((unsigned char)*q), q++;
  return p;
}

const char* xstring_lower(const xstring* s) {
  if (!s) return "";
  const char* p = s->s ? s->s : s->b;
  char* q = (char*)p;
  while (*q) *q = tolower((unsigned char)*q), q++;
  return (const char*)p;
}

xbool_t xstring_equal_ex(const xstring* s1, const char* s2, int flag) {
  if (!s1 || !s2) return xFALSE;
  const char* s1_str = xstring_to_string(s1);
  if (s1->len != strlen(s2)) return xFALSE;
  if (flag == X_NOCASE) {
    return strncasecmp(s1_str, s2, s1->len) == 0;
  } else {
    return strncmp(s1_str, s2, s1->len) == 0;
  }
}

xbool_t xstring_has_prefix_ex(const xstring* s, const char* prefix, int flag) {
  if (!s || !prefix) return xFALSE;
  size_t prefix_len = strlen(prefix);
  if (s->len < prefix_len) return xFALSE;
  const char* s_str = xstring_to_string(s);
  if (flag == X_NOCASE) {
    return strncasecmp(s_str, prefix, prefix_len) == 0;
  } else {
    return strncmp(s_str, prefix, prefix_len) == 0;
  }
}

xbool_t xstring_has_suffix_ex(const xstring* s, const char* suffix, int flag) {
  if (!s || !suffix) return xFALSE;
  size_t suffix_len = strlen(suffix);
  if (s->len < suffix_len) return xFALSE;
  const char* s_str = xstring_to_string(s);
  const char* start = s_str + (s->len - suffix_len);
  if (flag == X_NOCASE) {
    return strncasecmp(start, suffix, suffix_len) == 0;
  } else {
    return strncmp(start, suffix, suffix_len) == 0;
  }
}

xbool_t xstring_has_charset_ex(const xstring* s,
                               const char* charset,
                               int flag) {
  if (!s || !charset) return xFALSE;
  const char* s_str = xstring_to_string(s);
  if (flag == X_NOCASE) {
    char* temp_s = xbox_strdup(s_str);
    char* temp_charset = xbox_strdup(charset);
    if (!temp_s || !temp_charset) {
      if (temp_s) xbox_free(temp_s);
      if (temp_charset) xbox_free(temp_charset);
      return xFALSE;  // Out of memory
    }
    for (char* p = temp_s; *p; ++p) *p = tolower(*p);
    for (char* p = temp_charset; *p; ++p) *p = tolower(*p);
    xbool_t found = (strpbrk(temp_s, temp_charset) != NULL);
    xbox_free(temp_s);
    xbox_free(temp_charset);
    return found;
  } else {
    return strpbrk(s_str, charset) != NULL;
  }
}

xbool_t xstring_has_substr_ex(const xstring* s, const char* substr, int flag) {
  if (!s || !substr) return xFALSE;
  const char* s_str = xstring_to_string(s);
  if (flag == X_NOCASE) {
#ifdef _GNU_SOURCE
    return strcasestr(s_str, substr) != NULL;
#else
    const char* p = s_str;
    size_t substr_len = strlen(substr);
    while (*p) {
      if (strncasecmp(p, substr, substr_len) == 0) {
        return xTRUE;
      }
      p++;
    }
    return xFALSE;
#endif
  } else {
    return strstr(s_str, substr) != NULL;
  }
}

static const char* strpbrk_continue(const char* s, const char* set) {
  const char* p = strpbrk(s, set);
  for (; p && *p;) {
    if (strchr(set, *(p + 1)))
      p++;
    else
      break;
  }

  return p;
}

int xstring_tokenize_by_charset(const xstring* s,
                                const char* charset,
                                const char** token) {
  if (!s || !charset || !token) {
    return 0;
  }

  if (*token == NULL) {
    // Start from the beginning of the string
    *token = (s->s ? s->s : s->b);
    const char* p = strpbrk(*token, charset);
    if (p)
      return p - *token;
    else
      return strlen(*token);
  } else {
    const char* p = strpbrk_continue(*token, charset);
    if (p) {
      *token = p + 1;
      p = strpbrk(*token, charset);
      if (p)
        return p - *token;
      else
        return strlen(*token);
    } else {
      *token = NULL;
      return 0;
    }
  }
}

int xstring_tokenize_by_substr(const xstring* s,
                               const char* substr,
                               const char** token) {
  if (!s || !substr || !token) {
    return 0;
  }

  if (*token == NULL) {
    // Start from the beginning of the string
    *token = (s->s ? s->s : s->b);
    const char* p = strstr(*token, substr);

    if (p == *token) {
      *token += strlen(substr);
      p = strstr(*token, substr);
    }

    if (p)
      return p - *token;
    else
      return strlen(*token);
  } else {
    const char* p = strstr(*token, substr);
    if (p) {
      *token = p + strlen(substr);
      p = strstr(*token, substr);
      if (p)
        return p - *token;
      else
        return strlen(*token);
    } else {
      *token = NULL;
      return 0;
    }
  }
}

// KMP helper function to compute the Longest Proper Prefix which is also a
// Suffix
static void compute_lps_array(const char* pattern, int M, int* lps) {
  int length = 0;
  lps[0] = 0;
  int i = 1;
  while (i < M) {
    if (pattern[i] == pattern[length]) {
      length++;
      lps[i] = length;
      i++;
    } else {
      if (length != 0) {
        length = lps[length - 1];
      } else {
        lps[i] = 0;
        i++;
      }
    }
  }
}

const char* xstring_replace(xstring* s,
                            const char* old_str,
                            const char* new_str) {
  if (!s || !old_str || !new_str || xstring_is_empty(s)) {
    return xstring_to_string(s);
  }

  int old_len = strlen(old_str);
  if (old_len == 0) {
    return xstring_to_string(s);
  }
  int new_len = strlen(new_str);
  const char* s_str = xstring_to_string(s);

  // Use KMP to find all occurrences of old_str
  int M = old_len;
  int N = s->len;
  int* lps = (int*)xbox_malloc(sizeof(int) * M);
  if (!lps) return s_str;  // Malloc failed
  compute_lps_array(old_str, M, lps);

  // Store indices of matches
  int* matches = (int*)xbox_malloc(sizeof(int) * N);
  if (!matches) {
    xbox_free(lps);
    return s_str;  // Malloc failed
  }

  int i = 0, j = 0, count = 0;
  while (i < N) {
    if (old_str[j] == s_str[i]) {
      i++;
      j++;
    }
    if (j == M) {
      if (count < N) {
        matches[count++] = i - j;
      }
      j = lps[j - 1];
    } else if (i < N && old_str[j] != s_str[i]) {
      if (j != 0)
        j = lps[j - 1];
      else
        i++;
    }
  }
  xbox_free(lps);

  if (count == 0) {
    xbox_free(matches);
    return s_str;
  }

  // Calculate new length and allocate a new buffer
  // Use long long to prevent overflow during calculation
  long long new_s_len_ll = (long long)s->len +
                           (long long)count * (new_len - old_len);
  if (new_s_len_ll < 0) new_s_len_ll = 0;
  size_t new_s_len = (size_t)new_s_len_ll;

  char* new_buffer = (char*)xbox_malloc(new_s_len + 1);
  if (!new_buffer) {
    xbox_free(matches);
    return s_str;
  }

  // Build the new string in the new buffer
  char* p = new_buffer;
  int last_pos = 0;
  for (int k = 0; k < count; k++) {
    int match_pos = matches[k];
    // Copy segment before the match
    memcpy(p, s_str + last_pos, match_pos - last_pos);
    p += match_pos - last_pos;
    // Copy the new string
    memcpy(p, new_str, new_len);
    p += new_len;
    last_pos = match_pos + old_len;
  }
  // Copy the remaining part of the string after the last match
  memcpy(p, s_str + last_pos, s->len - last_pos);
  new_buffer[new_s_len] = '\0';

  xbox_free(matches);

  if (new_s_len < s->cap) {
    memcpy(s->s ? s->s : s->b, new_buffer, new_s_len + 1);
    xbox_free(new_buffer);
  } else {
    if (s->s) xbox_free(s->s);
    s->s = new_buffer;
    s->cap = new_s_len + 1;
  }
  s->len = new_s_len;

  return xstring_to_string(s);
}

int xstring_stoi(const xstring* s, int base) {
  if (!s || xstring_is_empty(s)) return 0;
  const char* p = xstring_to_string(s);

  // Skip whitespace
  while (*p && isspace((unsigned char)*p)) p++;

  int sign = 1;
  if (*p == '-') {
    sign = -1;
    p++;
  } else if (*p == '+') {
    p++;
  }

  // Auto-detect base if 0
  if (base == 0) {
    if (*p == '0') {
      if (*(p + 1) == 'x' || *(p + 1) == 'X') {
        base = 16;
        p += 2;
      } else {
        base = 8;
        p++;
      }
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
      p += 2;
    }
  }

  long long result = 0;
  while (*p) {
    int digit;
    if (isdigit((unsigned char)*p)) {
      digit = *p - '0';
    } else if (isalpha((unsigned char)*p)) {
      digit = toupper((unsigned char)*p) - 'A' + 10;
    } else {
      break;
    }

    if (digit >= base) break;

    result = result * base + digit;
    p++;
  }

  return (int)(result * sign);
}

double xstring_stod(const xstring* s) {
  if (!s || xstring_is_empty(s)) return 0.0;
  const char* p = xstring_to_string(s);

  while (*p && isspace((unsigned char)*p)) p++;

  double sign = 1.0;
  if (*p == '-') {
    sign = -1.0;
    p++;
  } else if (*p == '+') {
    p++;
  }

  double result = 0.0;
  while (*p && isdigit((unsigned char)*p)) {
    result = result * 10.0 + (*p - '0');
    p++;
  }

  if (*p == '.') {
    p++;
    double fraction = 1.0;
    while (*p && isdigit((unsigned char)*p)) {
      fraction /= 10.0;
      result += (*p - '0') * fraction;
      p++;
    }
  }

  // Handle exponent e/E
  if (*p == 'e' || *p == 'E') {
    p++;
    int exp_sign = 1;
    if (*p == '-') {
      exp_sign = -1;
      p++;
    } else if (*p == '+') {
      p++;
    }

    int exponent = 0;
    while (*p && isdigit((unsigned char)*p)) {
      exponent = exponent * 10 + (*p - '0');
      p++;
    }

    double power = 1.0;
    for (int k = 0; k < exponent; k++) {
      power *= 10.0;
    }

    if (exp_sign == 1) {
      result *= power;
    } else {
      result /= power;
    }
  }

  return result * sign;
}

xstring xstring_itos(int val) {
  char buf[32];
  int i = 0;
  int sign = val < 0;
  unsigned int n = (unsigned int)(sign ? -val : val);

  if (val == 0) {
    return xstring_init_iter("0");
  }

  while (n > 0) {
    buf[i++] = (n % 10) + '0';
    n /= 10;
  }

  if (sign) {
    buf[i++] = '-';
  }

  buf[i] = '\0';

  // Reverse
  for (int j = 0; j < i / 2; j++) {
    char temp = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = temp;
  }

  return xstring_init_iter(buf);
}

xstring xstring_dtos(double val) {
  if (val == 0.0) return xstring_init_iter("0.000000");

  char buf[64];
  int i = 0;

  if (val < 0) {
    buf[i++] = '-';
    val = -val;
  }

  long long int_part = (long long)val;
  double frac_part = val - int_part;

  // Integer part
  char int_buf[32];
  int j = 0;
  if (int_part == 0) {
    int_buf[j++] = '0';
  } else {
    long long temp = int_part;
    while (temp > 0) {
      int_buf[j++] = (temp % 10) + '0';
      temp /= 10;
    }
  }
  while (j > 0) buf[i++] = int_buf[--j];

  buf[i++] = '.';

  // Fractional part (6 decimal places)
  for (int k = 0; k < 6; k++) {
    frac_part *= 10;
    int digit = (int)frac_part;
    buf[i++] = digit + '0';
    frac_part -= digit;
  }
  buf[i] = '\0';

  return xstring_init_iter(buf);
}
