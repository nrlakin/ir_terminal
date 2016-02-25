#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "ir_sensor.h"
#include "config.h"

volatile rx_status_t IrRxStatus;
volatile unsigned char pinstate;

// Macro for reading level on INT0; active low.
#define CommRx  ((PIND & (1<<PIND2))!=0)
#define NOP  __asm__ __volatile__ ("nop\n\t")

// Local function prototypes.
void StartTimer(void);
void StopTimer(void);
void initTimer2(void);

/*
 *  This ISR is responsible for handling external interrupts from the IR sensor. It 
 *  is responsible for forming edges into bits, and bits into bytes.
 *  
 *
 */
ISR(INT0_vect) {
#ifdef DEBUG
  digitalWrite(4, digitalRead(2));
#else
  RXStatus.width = TCNT2;        // capture time
  TCNT2 = 0;                     // reset timer
  StartTimer();         // start timer, in case of down transition
  NOP;                        // short delay for debouncing
  NOP;
  RXStatus.level = CommRx;      // now check and see type of transition
  if(RXStatus.level) {
    RXStatus.new_bit = true;
  }
#endif
}

ISR(TIMER2_OVF_vect) {
 #ifdef DEBUG
 if (pinstate) pinstate=0;
 else pinstate=1;
 digitalWrite(5, pinstate);
 #else
  bool started;                  //remember this bit
  StopTimer();
  TCNT2=0;                      // clear counter (not necessary if not using output compare)
  RXStatus.result = 0;
  started = RXStatus.started;
  RXStatus.flags = 0;
  if(started) RXStatus.started=true;
  RXStatus.timeout = true;
#endif
}

/* 
 * Configure Timer2 as "stopwatch" to measure pulse widths.
 */
void initTimer2(void) {
  TCCR2A = 0x00;        // no output compare, timer in normal counter mode.
  TCCR2B = 0x00;        // timer off for now
  TCNT2 = 0;            // clear timer
//  TIFR2 |= 0x01;        // clear timer2 overflow flag
  TIMSK2 |= 0x01;       // enable timer 2 overflow interrupt.
}

/*
 * Configure external interrupt, timer, and status variables for IR sensor.
 */
void initIrSensor(unsigned char iopin, unsigned char protocol) {
  pinMode(2, INPUT_PULLUP);
  EICRA |= 0x01;    // trigger interrupts on any edge of int0.
  EIMSK |= 0x01;    // enable INT0
  EIFR = 0;         // clear interrupt flags
  initTimer2();
#ifdef DEBUG
  pinstate=1;
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5, pinstate);
  StartTimer();
#else
  rxState.currentByte = 0;
  rxState.bitIndex = 0;
  rxState.rxState = RX_IDLE;
#endif
}

/*
 * Start timer with 1024 prescaler.
 */
void StartTimer(void) {
  TCCR2B |= 0x07;
}

/*
 * Stop timer by clearing CS bits.
 */
void StopTimer(void) {
  TCCR2B &= 0xF8;  // clear 3 CS22:20
}
