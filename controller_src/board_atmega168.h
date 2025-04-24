/**
  * Definition for the Arduino UNO board.
  * Contains pins
  */

// interrupt pins used for the light barrier
const uint8_t PIN_INT_0 = 0;
const uint8_t PIN_INT_1 = 1;

// pin for the external push-button
const uint8_t PIN_BUTTON = 2;

// display drivers shared pins
const uint8_t PIN_BCD_A = 8;
const uint8_t PIN_BCD_B = 9;
const uint8_t PIN_BCD_C = 10;
const uint8_t PIN_BCD_D = 11 ;
const uint8_t PIN_nLT = 15; // light test
const uint8_t PIN_nBI = 14; // blank
// individual latch enable pins
const uint8_t PIN_LE_0 = 16;
const uint8_t PIN_LE_1 = 17;
const uint8_t PIN_LE_2 = 18;
const uint8_t PIN_LE_3 = 19;

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
  PCMSK2 |= 1 << interruptNum;
}
void detachInterrupt(uint8_t interruptNum) {
  PCMSK2 &= ~(1 << interruptNum);
}
// also overwrite this macro
#define digitalPinToInterrupt(p) (p)


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

