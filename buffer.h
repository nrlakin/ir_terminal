#ifndef _BUFFER_H_
#define _BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_SIZE  64

typedef struct {
  unsigned char length;
  unsigned char *in;
  unsigned char *out;
  unsigned char buffer[BUFFER_SIZE];
} buffer_t;

void initBuff(buffer_t *buffPtr);
void putBuff(unsigned char val, buffer_t *dest);  

#ifdef __cplusplus
}
#endif

#endif
