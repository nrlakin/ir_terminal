#include <Arduino.h>
#include "avr/io.h"

void WriteUart(unsigned char val) {
  while(UCSR0A & (1 << UDRE0));
  UDR0 = val;
}
