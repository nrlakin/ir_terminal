#ifndef _IR_SENSOR_H_
#define _IR_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

typedef struct {
  bool level;
  bool new_edge;
  bool new_packet;
  bool timeout;
  bool started;
  bool space;
  unsigned char bit_index;
  unsigned char width;
  unsigned char result;
} rx_status_t;

extern volatile rx_status_t IrRxStatus;
extern buffer_t RxBuffer;

void initIrSensor(unsigned char iopin, unsigned char protocol);
void processEdge(void);
//void processPacket(void);
void processTimeout(void);


#ifdef __cplusplus
}
#endif

#endif

