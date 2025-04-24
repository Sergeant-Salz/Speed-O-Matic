/**
  * Definition for the Arduino UNO board.
  * Contains pins
  */

// interrupt pins used for the light barrier
const uint8_t PIN_INT_0 = 2;
const uint8_t PIN_INT_1 = 3;

// display drivers shared pins
const uint8_t PIN_BCD_A = 12;
const uint8_t PIN_BCD_B = 13;
const uint8_t PIN_BCD_C = 4;
const uint8_t PIN_BCD_D = 5;
const uint8_t PIN_nLT = 6; // light test
const uint8_t PIN_nBI = 7; // blank
// individual latch enable pins
const uint8_t PIN_LE_0 = 8;
const uint8_t PIN_LE_1 = 9;
const uint8_t PIN_LE_2 = 10;
const uint8_t PIN_LE_3 = 11;

/**
 * Board specific setup.
 */
void boardSpecificSetup() {
  // as the interrupt pins not external interrupts they need special attention
  // they lie on PORTD (PD0/1)
  // activate interrupts on PD
  PCICR |= B00000100;
}

// store the direction that interrupts should trigger
int interruptMode = RISING;

// overwrite the default attachInterrupts routine
void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode) {
  interruptMode = mode;
  // user function is ignored
  // interruptNum ignored too
  // enable interrupt pins based on above constant
  // set mask to active interrupt pins
  PCMSK2 |= B00000011;
}
void detachInterrupt(uint8_t interruptNum) {
  PCMSK2 &= ~B00000011;
}

// these will be defined later in the source 
extern void triggerInt0();
extern void triggerInt1();

// PCINT ISR which will detect which pin changed and in what 'direction'
ISR (PCINT2_vect) {
  if (digitalRead(PIN_INT_0) == (interruptMode == RISING))
    triggerInt0();
  if (digitalRead(PIN_INT_1) == (interruptMode == RISING))
    triggerInt1();
}