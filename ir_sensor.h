#ifndef _IR_SENSOR_H_
#define _IR_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bool level;
  bool new_bit;
  bool new_byte;
  bool timeout;
  bool started;
  unsigned char bit_index;
  unsigned char width;
  unsigned char result;
} rx_status_t;

extern volatile rx_status_t IrRxStatus;

void initIrSensor(unsigned char iopin, unsigned char protocol);


#ifdef __cplusplus
}
#endif

#endif

