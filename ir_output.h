#ifndef _IR_OUTPUT_H_
#define _IR_OUTPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

void initIrOutput(unsigned char iopin, unsigned char protocol);
void TxIRPacket(unsigned char * packet, unsigned char len);


#ifdef __cplusplus
}
#endif

#endif

