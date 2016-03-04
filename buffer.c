#include "buffer.h"

void initBuff(buffer_t *buffPtr) {
  buffPtr->length = 0;
  buffPtr->in = buffPtr->buffer;
  buffPtr->out = buffPtr->buffer;
}

void putBuff(unsigned char val, buffer_t *dest) {
  if (dest->length == BUFFER_SIZE) return;
  *(dest->in) = val;
  dest->length++;
  dest->in++;
}
