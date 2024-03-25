#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

void int2str(char* str, int d) {
  size_t j = 0;

  do {
    str[j ++] = '0' + (d % 10);
    d /= 10; 
  } while(d > 0);

  size_t k;
  for (k = 0; k < j / 2; k ++) {
    char t = str[k];
    str[k] = str[j - k - 1];
    str[j - k - 1] = t;
  }
  str[j] = '\0';
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  char *p;

  // Initialize the va_list
  va_start(args, fmt);

  // Process the format string
  for (p = out; *fmt; fmt++) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case 'd': {
          int i = va_arg(args, int);
          char str[50];
          char *s = str;
          int2str(str, i);
          while (*s != '\0') {
            *p++ = *s ++;
          }
          break;
        }
        case 's': {
          char *s = va_arg(args, char *);
          while (*s != '\0') {
            *p++ = *s ++;
          }
          break;
        }
        default:
          *p++ = *fmt;
          break;
      }
    } else {
      *p++ = *fmt;
    }
  }

  *p = '\0';

  // Clean up the va_list
  va_end(args);

  // Return the length of the output string
  return p - out;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
