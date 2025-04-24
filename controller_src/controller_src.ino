// include pin mapping for the respective chip
#include "board_arduino_uno.h"
// direction of the interrupt to await
#define INTERRUPT_DIR RISING

// makros for serial debugging
#define ENABLE_SERIAL_DEBUG
#ifdef ENABLE_SERIAL_DEBUG
  #define DEBUG_PRINT(VAL) (Serial.print(VAL))
  #define DEBUG_PRINTLN(VAL) (Serial.println(VAL))
#else
  #define DEBUG_PRINT(VAL) ({})
  #define DEBUG_PRINTLN(VAL) ({})
#endif


/**
  * State machine states.
  */
enum FSMState {
  INIT, // any inital state
  CAPTURE, // both barriers have been set, ready to capture interrupts
  TRIGGERED_0, // INT0 triggered
  CAPTURE_DONE, // both interrupts have triggerd and result calculation is done
  OOO_ERROR, // interrupt have happened out of order
  GENERAL_ERROR // other error cases
};

// error codes to display
enum ErrorCodes {
  ERR_ILLEGAL_STATE = 2,
  ERR_UNKNOWN = 1,
};


/**
  * Control a single driver chip to display a given value.
  * Digits are numbered right-to-left. 
  * @param digit Digit to update. Must be within 0-3.
  * @param value Binary coded value to display. Must be within 0-9.
  */
void setDisplayDigit(uint8_t digit, uint8_t value);
/**
  * Controls the entire display to show a number.
  * @param value Binary coded value to display.
  */ 
void setDisplayValue(uint16_t value);
/**
  * ISR for the interrupts.
  * Sets current time and triggerd flag.
  */
void triggerInt0();
void triggerInt1();
/**
  * Handle the core loop logic for state INIT.
  * In this state, digits 0 and 3 display 0/1 respectively, 
  * indicating which of the barriers is properly setup.
  * If both pins are in the proper state for a duration, transition to state CAPTURE.
  */
void stateINIT();
/**
  * Handle the core loop logic for state CAPTURE.
  * Display 0 on all displays. Check if an interrupt has been triggered and
  * transition to next state if so (TRIGGERED_0 or OOO_ERROR).
  */
void stateCAPTURE();
/**
  * Handle the core loop logic for state TRIGGERED_0.
  * Display time since the first interrupt.
  * Transition to CAPTURE_DONE if interrupt 1 flag is set.
  */
void stateTRIGGERED();
/**
  * Handle the core loop logic for state CAPTURE_DONE.
  * Display the time delta between the two interrupts.
  */
void stateCAPTURE_DONE();
/**
  * Handle the core loop logic for state OOO_ERROR.
  * Display a 9 on all displays.
  */
void stateOOO_ERROR();
/**
  * Handle the core loop logic for state GENERAL_ERROR.
  * Display the error code on the leftmost digit.
  */
void stateGENERAL_ERROR(uint8_t errCode);

// variables to store time in interrupts
// stores time in microsecounds
// ref: https://docs.arduino.cc/language-reference/en/functions/time/micros/
volatile unsigned long int0Time, int1Time;
volatile bool int0Triggered = false, int1Triggered = false;

// state machine
// not volatile, as is should be managed in the main loop
FSMState state = INIT;

// time variable for state INIT time measurment
// always reset to 0 after exiting the state
unsigned long initTimer = 0;

// flag to indicate that the software reset has been issued
// will reset FSM to INIT
volatile bool fsmResetTriggered = 0;


void setup() {
  // setup pins
  pinMode(PIN_INT_0, INPUT);
  pinMode(PIN_INT_1, INPUT);
  pinMode(PIN_BCD_A, OUTPUT);
  pinMode(PIN_BCD_B, OUTPUT);
  pinMode(PIN_BCD_C, OUTPUT);
  pinMode(PIN_BCD_D, OUTPUT);
  pinMode(PIN_LE_0, OUTPUT);
  pinMode(PIN_LE_1, OUTPUT);
  pinMode(PIN_LE_2, OUTPUT);
  pinMode(PIN_LE_3, OUTPUT);
  pinMode(PIN_nLT, OUTPUT);
  pinMode(PIN_nBI, OUTPUT);

  // set default value for some outputs
  digitalWrite(PIN_nLT, HIGH);
  digitalWrite(PIN_nBI, HIGH);
  digitalWrite(PIN_LE_0, HIGH);
  digitalWrite(PIN_LE_1, HIGH);
  digitalWrite(PIN_LE_2, HIGH);
  digitalWrite(PIN_LE_3, HIGH);

  // setup FSM
  state = INIT;

  // run board-specific setup
  boardSpecificSetup();

  // enable serial if debugging is enabled
  #ifdef ENABLE_SERIAL_DEBUG
    Serial.begin(9600);
    Serial.print("Starting serial debug...\n");
    Serial.flush();
  #endif
}

void loop() {
  // always have the option to reset the FSM to INIT
  if (fsmResetTriggered) {
    state = INIT;
    DEBUG_PRINT("FSM Reset into INIT\n");
  }

  // execute the respective routine
  switch(state) {
    case INIT:
      stateINIT();
      break;
    case CAPTURE:
      stateCAPTURE();
      break;
    case TRIGGERED_0:
      stateTRIGGERED();
      break;
    case CAPTURE_DONE:
      stateCAPTURE_DONE();
      break;
    case OOO_ERROR:
      stateOOO_ERROR();
      break;
    case GENERAL_ERROR:
      stateGENERAL_ERROR(ERR_UNKNOWN);
    default:
      stateGENERAL_ERROR(ERR_ILLEGAL_STATE);
  }

}
