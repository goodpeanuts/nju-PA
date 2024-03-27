#include "stlt/vector.h"

void resize_buf(void** buf, unsigned long size, size_t* buf_cnt) {
  if (*buf_cnt == 0) {
    *buf = malloc(size * BUF_SIZE);
  } 
  else if (*buf_cnt % BUF_SIZE == 0) {
    *buf = realloc(*buf, size * ((*buf_cnt) + BUF_SIZE));
  } 
}