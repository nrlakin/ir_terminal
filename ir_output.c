#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"

#ifdef NEC_PROTOCOL
  #define ONE_SPACE    1687
  #define ZERO_SPACE   562
  #define MARK         562
  #define START_MARK   9000
  #define START_SPACE  4500
  #define STOP_MARK    562
#elif defined PANASONIC
  #define ONE_SPACE    1320
  #define ZERO_SPACE   450
  #define MARK         424
  #define START_MARK   3508
  #define START_SPACE  1756
  #define STOP_MARK    450
#endif

void togglePWM(bool on) {
  if (on) TCCR1A |= 0x40;
  else TCCR1A &= ~0x40;
}

void initIrOutput(unsigned char iopin, unsigned char protocol) {
  DDRB |= _BV(DDB1);  // set Port B1 to output.
  PORTB &= ~(_BV(PORTB1));  // set low.
  OCR1AH = 0;
  OCR1AL = 13;
  TCCR1A = 0x01;    // don't turn on yet.
  TCCR1B = 0x12;
  TCCR1C = 0;
  TIMSK1 = 0;
  TIFR1 = 0;
}

void txByte(unsigned char outByte) {
  unsigned char mask = 0x01;
  unsigned char i;
  unsigned int space;
  for (i=0; i<8; i++) {
    if(outByte&mask) space = ONE_SPACE;
    else space = ZERO_SPACE;
    togglePWM(true);
    delayMicroseconds(MARK);
    togglePWM(false);
    delayMicroseconds(space);
    mask <<= 1;
  }
}

void txStart(void) {
  togglePWM(true);
  delayMicroseconds(START_MARK);
  togglePWM(false);
  delayMicroseconds(START_SPACE);
}

void txStop(void) {
  togglePWM(true);
  delayMicroseconds(STOP_MARK);
  togglePWM(false);
}

void TxIRPacket(unsigned char * packet, unsigned char len) {
  unsigned char i;
  txStart();
  for (i=0; i<len; i++) {
    txByte(packet[i]);
    //txByte(~packet[i]);
  }
  txStop();
}
  
