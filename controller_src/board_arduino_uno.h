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
  // nothing to be done for the arduino
}