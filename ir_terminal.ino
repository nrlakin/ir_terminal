#include <avr/interrupt.h>
#include "config.h"
#include "ir_sensor.h"
#include "ir_output.h"

unsigned char PanasonicPowerSeq[6] = {0x02, 0x20, 0x80, 0x00, 0x3E, 0xBE};

void PowerPanasonic(void) {
  TxIRPacket(PanasonicPowerSeq, 6);
}

void setup() {
  // put your setup code here, to run once:
  initIrSensor(0, 0);
  initIrOutput(0, 0);
  Serial.begin(9600);
  sei();
}

void loop() {
  unsigned char i;
//  delay(15000);
//  for(i=0; i<4; i++) {
//    PowerPanasonic();
//    delay(75);
//  }
  if(Serial.available()) {
    i = Serial.read();
    TxIRPacket(&i, 1);
  }
}
