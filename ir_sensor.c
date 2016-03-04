#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "ir_sensor.h"
#include "buffer.h"


volatile rx_status_t IrRxStatus;
volatile unsigned char pinstate;
buffer_t RxBuffer;

// Macro for reading level on INT0; active low.
#define CommRx  ((PIND & (1<<PIND2))==0)
#define NOP  __asm__ __volatile__ ("nop\n\t")

#define RX_BUFFER_SIZE    64

// Bit width constants
#ifdef NEC_PROTOCOL
  #define ONE_SPACE_MIN      1602
  #define ONE_SPACE_RNG      169
  #define ZERO_SPACE_MIN     534
  #define ZERO_SPACE_RNG     56 
  #define MARK_MIN           534
  #define MARK_RNG           56
  #define START_MARK_MIN     8550
  #define START_MARK_RNG     900
  #define START_SPACE_MIN    4275
  #define START_SPACE_RNG    450
  #define STOP_MARK_MIN      534
  #define STOP_MARK_RNG      56
  #define MIN_STOP_SPACE     15000
#elif defined PANASONIC
  #define ONE_SPACE    1320
  #define ZERO_SPACE   450
  #define MARK         424
  #define START_MARK   3508
  #define START_SPACE  1756
  #define STOP_MARK    450
#endif

// Local function prototypes.
void StartTimer(void);
void StopTimer(void);
//void initTimer2(void);
void InitTimerRx(void);
void ResetIrRxStatus(void);

struct IrRxPacket {
  unsigned char length;
  unsigned char *next;
  unsigned char buffer[RX_BUFFER_SIZE];
} IrRxPacket;

/*
 *  This ISR is responsible for handling external interrupts from the IR sensor. It 
 *  is responsible for forming edges into bits, and bits into bytes.
 *  
 *
 */
ISR(INT0_vect) {
#ifdef DEBUG
  digitalWrite(4, CommRx);
#else
  IrRxStatus.width = TCNT2;       // capture time
//  TCNT2 = 0;                      // reset timer
  TCNT1 = 0;                      // reset timer
  StartTimer();                   // start timer, in case of down transition
  NOP;                            // short delay for debouncing
  NOP;
  IrRxStatus.level = CommRx;      // now check and see type of transition
  IrRxStatus.new_edge = true;
#endif
}

//ISR(TIMER2_OVF_vect) {
ISR(TIMER1_COMPA_vect) {
 #ifdef DEBUG
 if (pinstate) pinstate=0;
 else pinstate=1;
 digitalWrite(5, pinstate);
 #else
  bool started;                  //remember this bit
  StopTimer();
//  TCNT2=0;                      // clear counter (not necessary if not using output compare)
  TCNT1 = 0;
  IrRxStatus.result = 0;
  IrRxStatus.timeout = true;
#endif
}

/* 
 * Configure Timer2 as "stopwatch" to measure pulse widths.
 * Will roll over every 256*(1024/16MHz) = 16.38ms
 * One tick = 64us
 */
void InitTimer2(void) {
  TCCR2A = 0x00;        // no output compare, timer in normal counter mode.
  TCCR2B = 0x00;        // timer off for now
  TCNT2 = 0;            // clear timer
//  TIFR2 |= 0x01;        // clear timer2 overflow flag
  TIMSK2 |= 0x01;       // enable timer 2 overflow interrupt.
}

void InitTimerRx(void) {
  TCCR1A = 0x00;        // CTC mode
  TCCR1B = 0x08;        // CTC, TOP = OCR1A, timer off for now.
  TCNT1 = 0;            // clear counter
  OCR1A = 5000;         // with 64:1 ps, each tick is 4us, so compare match at 20ms
  TIMSK1 |= 0x02;        // enable compare match interrupt.
}
  
  
/*
 * Configure external interrupt, timer, and status variables for IR sensor.
 */
void initIrSensor(unsigned char iopin, unsigned char protocol) {
  pinMode(2, INPUT_PULLUP);
  EICRA |= 0x01;    // trigger interrupts on any edge of int0.
  EIMSK |= 0x01;    // enable INT0
  EIFR = 0;         // clear interrupt flags
  InitTimerRx();
#ifdef DEBUG
  pinstate=1;
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5, pinstate);
  StartTimer();
#else
  ResetIrRxStatus();
#endif
}

void ResetIrRxStatus(void) {
  IrRxStatus.level = false;
  IrRxStatus.new_edge = false;
  IrRxStatus.new_packet = false;
  IrRxStatus.timeout = false;
  IrRxStatus.started = false;
  IrRxStatus.space = false;
  IrRxStatus.bit_index = 0;
  IrRxStatus.width = 0;
  IrRxStatus.result = 0;
  initBuff(&RxBuffer);
}

/*
 * Start timer with 1024 prescaler.
 */
void StartTimer(void) {
//  TCCR2B |= 0x07;
  TCCR1B |= 0x03;
}

/*
 * Stop timer by clearing CS bits.
 */
void StopTimer(void) {
//  TCCR2B &= 0xF8;  // clear 3 CS22:20
  TCCR1B &= 0xF8;  // clear 3 CS22:20
}

void processTimeout(void) {
  IrRxStatus.timeout = 0;
  if (IrRxStatus.level) {
    // if high, there's an error.
    initIrSensor(0,0);
    return;
  }
  if (IrRxStatus.started) {
    if (IrRxStatus.space) {
      // possibly a stop bit...
      if (IrRxStatus.bit_index == 0) {
        IrRxStatus.new_packet = true;
        return;
      }
    }
  }
  initIrSensor(0,0);
  return;
}

void processEdge(void) {
    bool level = IrRxStatus.level;
    unsigned int width = IrRxStatus.width;
    IrRxStatus.new_edge=0;
    if (IrRxStatus.new_packet) return;    // ignore until we've processed last packet.
    if (level == false) { // Line went low. Check if last interval was valid mark; if so, this is a space.
      if (IrRxStatus.started) {
        if ((unsigned int)(width-MARK_MIN) <= MARK_RNG) IrRxStatus.space = true;
        else if ((unsigned int)(width-STOP_MARK_MIN) <= STOP_MARK_RNG) IrRxStatus.space = true;
      } else {
        if ((unsigned int)(width-START_MARK_MIN) <= START_MARK_RNG) IrRxStatus.space = true;
      }
      return;
    } else {
      if (IrRxStatus.space == false) {
        ResetIrRxStatus();
        return;
      }
    }
    // If code reaches here, we have just completed a "space" and can process meaning.
    IrRxStatus.space = false;
    if (IrRxStatus.started == false) {
      if ((unsigned int)(width-START_SPACE_MIN) <= START_SPACE_RNG) IrRxStatus.started = true;
      else {
        ResetIrRxStatus();
      }
      return;
    } else {
      if ((unsigned int)(width-ZERO_SPACE_MIN) <= ZERO_SPACE_RNG) {
        IrRxStatus.bit_index++;
      } else if ((unsigned int)(width-ONE_SPACE_MIN) <= ONE_SPACE_RNG) {
        IrRxStatus.result |= (0x01 << IrRxStatus.bit_index);
        IrRxStatus.bit_index++;
      } else {
        if (width >= MIN_STOP_SPACE) {
          if (IrRxStatus.bit_index == 0) IrRxStatus.new_packet = true;
          else ResetIrRxStatus();
        }
        ResetIrRxStatus();
        return;
      }
      if (IrRxStatus.bit_index == 8) {
        putBuff(IrRxStatus.result, &RxBuffer);
        IrRxStatus.bit_index = 0;
        IrRxStatus.result = 0;
      }
      return;
    }
}
        
//    if((RXStatus.width>=RX_START_LENGTH)||(RXStatus.width<RX_ONE_LENGTH)) {
//        RXStatus.result=0;
//        RXStatus.bit_index=0;
//        RXStatus.started=1;
//        RXResetPacket();
//        return;
//    }
////  if(RXStatus.width<ZERO_LENGTH) {
//    if(RXStatus.started==0)return;          //never saw a start bit; discard
//    if(RXStatus.width<RX_ZERO_LENGTH) {
//        if(RXStatus.width>=RX_ONE_LENGTH)RXStatus.result|=(0x80>>RXStatus.bit_index);
//    }
//    if(RXStatus.bit_index==7) {
//        RXStatus.new_byte=1;
//        RXStatus.bit_index=0;
//    } else {
//        RXStatus.bit_index++;
//    }
//}
