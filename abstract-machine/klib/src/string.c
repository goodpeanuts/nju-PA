#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s != 0) {
    len ++;
    s ++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  size_t i;

  for (i = 0; src[i] != '\0'; i++)
      dst[i] = src[i];

  dst[i] = '\0';

  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++)
      dst[i] = src[i];
  for ( ; i < n; i++)
      dst[i] = '\0';

  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  size_t i;

  for (i = 0 ; src[i] != '\0' ; i++)
      dst[dst_len + i] = src[i];
  dst[dst_len + i] = '\0';

  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
      s1++;
      s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i;
  size_t s1_len = strlen(s1);
  size_t s2_len = strlen(s2);
  if (s1_len < n) n = s1_len;
  if (s2_len < n) n = s2_len;
  
  for (i = 0; i < n; i ++) {
    if (s1[i] > s2[i]) return i;
    else if (s1[i] < s2[i]) return -i;
  }

  return 0;
}

void *memset(void *s, int c, size_t n) {
  char *p = (char*)s;
  for (size_t i = 0; i < n; i ++) {
    p[i] = (char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  void* buffer = malloc(n);
  char *in = (char*)src;
  char *out = (char*)buffer;
  for (size_t i = 0; i < n; i ++) {
    out[i] = in[i];
  }

  in = (char*)buffer;
  out = (char*)dst;
  for (size_t i = 0; i < n; i ++) {
    out[i] = in[i];
  }
  free(buffer);
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *s = (char*)in;
  char *d = (char*)out;
  for (size_t i = 0; i < n; i ++) {
    d[i] = s[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  size_t i;
  char *p1 = (char*) s1;
  char *p2 = (char*) s2;

  for (i = 0; i < n; i ++) {
    if (p1[i] > p2[i]) return i;
    else if (p1[i] < p2[i]) return -i;
  }

  return 0;
}

#endif
