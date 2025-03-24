/**
  * Implementation file for core functions.
  */

// time to wait in INIT after barrier setup is completed
constexpr unsigned long INIT_SETUP_DELAY_MS = 1000;

// approximate number of nanoseconds in every cycle
constexpr uint16_t NS_PER_CYCLE = (1000000000LU / F_CPU);

/**
  * Helper function that uses a NOP loop to delay for approximately the given time.
  * Accuracy depends on the used system clock and duration of NOP.
  * @param ns Number of nanosecounds to delay for at least. Might be more
  */
void nsDelay(uint16_t ns) {
  uint16_t cycles = ns / NS_PER_CYCLE;
  for(uint16_t i = 0; i < cycles; ++i)
    __asm__ __volatile__ ("nop\n\t");
}


/**
  * Blank a given digit on the display.
  * @param Digit to blank.
  * @note Setting a BCD value greater 9 will blank the display
  */
void blankDisplayDigit(uint8_t digit) {
  setDisplayDigit(digit, 255);
}

/**
  * Control a single driver chip to display a given value.
  * Digits are numbered right-to-left. 
  * @param digit Digit to update. Must be within 0-3.
  * @param value Binary coded value to display. Values greater than 9 are displayed as blank.
  */
void setDisplayDigit(uint8_t digit, uint8_t value) {
  if (digit > 3)
    return;

  // set BCD output pins
  digitalWrite(PIN_BCD_A, value & 0b0001);
  digitalWrite(PIN_BCD_B, value & 0b0010);
  digitalWrite(PIN_BCD_C, value & 0b0100);
  digitalWrite(PIN_BCD_D, value & 0b1000);

  // gate the latch enable 
  uint8_t digitLatchPin;
  switch (digit) {
    case 0:
      digitLatchPin = PIN_LE_0;
      break;
    case 1:
      digitLatchPin = PIN_LE_1;
      break;
    case 2:
      digitLatchPin = PIN_LE_2;
      break;
    case 3:
      digitLatchPin = PIN_LE_3;
      break;
    default:
      return;
  }
  // pull high
  digitalWrite(digitLatchPin, LOW);
  // satisfy hold time
  nsDelay(90);
  digitalWrite(digitLatchPin, HIGH);
}


/**
  * Controls the entire display to show a number.
  * @param value Binary coded value to display.
  */ 
void setDisplayValue(uint16_t value) {
  setDisplayDigit(0, value % 10);
  setDisplayDigit(1, (value / 10) % 10);
  setDisplayDigit(2, (value / 100) % 10);
  setDisplayDigit(3, (value / 1000) % 10);
}


/**
  * ISR for the interrupts.
  * Sets current time and triggerd flag.
  */
void triggerInt0() {
  int0Time = millis();
  int0Triggered = true;
  DEBUG_PRINT("Interrupt 0 at: ");
  DEBUG_PRINTLN(int0Time);
}
void triggerInt1() {
  int1Time = millis();
  int1Triggered = true;
  DEBUG_PRINT("Interrupt 1 at: ");
  DEBUG_PRINTLN(int1Time);
}

/**
  * Transiton into CAPTURE state.
  * Shows all 0's on the screen. Clears triggered flags.
  * Attaches ISR's and performs state change.
  */
void transitionToCPATURE() {
  // show all 0's
  setDisplayDigit(0, 0);
  setDisplayDigit(1, 0);
  setDisplayDigit(2, 0);
  setDisplayDigit(3, 0);

  // clear trigger flags
  int0Triggered = 0;
  int1Triggered = 0;

  // attach interrupts
  attachInterrupt(digitalPinToInterrupt(PIN_INT_0), triggerInt0, INTERRUPT_DIR);
  attachInterrupt(digitalPinToInterrupt(PIN_INT_1), triggerInt1, INTERRUPT_DIR);

  // set FSM state
  state = CAPTURE;

  DEBUG_PRINT("FSM transition into CAPTURE\n");
}

/**
  * Handle the core loop logic for state INIT.
  * In this state, digits 0 and 3 display 0/1 respectively, 
  * indicating which of the barriers is properly setup.
  * If both pins are in the proper state for a duration, transition to state CAPTURE.
  */
void stateINIT() {
  bool sig0 = digitalRead(PIN_INT_0);
  bool sig1 = digitalRead(PIN_INT_1);

  // display the state of the signals
  setDisplayDigit(0, sig1);
  blankDisplayDigit(1);
  blankDisplayDigit(2);
  setDisplayDigit(3, sig0);

  constexpr bool readySignalState = (INTERRUPT_DIR == FALLING) ? HIGH : LOW;

  // reset the INIT timer if any of the pins is not in ready state
  if (sig0 != readySignalState || sig1 != readySignalState)
    initTimer = 0;

  // if both pins are ready
  if (sig0 == readySignalState && sig1 == readySignalState) {
    // if the counter is 0, then set it to current millis
    if (initTimer == 0) {
      initTimer = millis();
      DEBUG_PRINT("INIT: Starting timer at ");
      DEBUG_PRINTLN(initTimer);
      return;
    }

    // if there is already a value in the time
    // and the required time has passed: transition state
    if (millis() - initTimer >= INIT_SETUP_DELAY_MS) {
      // reset timer
      initTimer = 0;
      // perform transition into the next state
      transitionToCPATURE();
      return;
    }
  }
}


/**
  * Handle the core loop logic for state CAPTURE.
  * Display 0 on all displays. Check if an interrupt has been triggered and
  * transition to next state if so (TRIGGERED_0 or OOO_ERROR).
  * Disable interrupts after exiting the state.
  */
void stateCAPTURE() {
  
  // if nothing has happend, return
  if (!int0Triggered && !int1Triggered)
    return;

  // it might happen that both interrupts trigger between an iteration of the loop
  // in that case, decide the next based on the time delta
  if (int0Triggered && int1Triggered) {
    detachInterrupt(digitalPinToInterrupt(PIN_INT_0));
    detachInterrupt(digitalPinToInterrupt(PIN_INT_1));
    if (int1Time - int0Time >= 0) {
      state = CAPTURE_DONE;
      DEBUG_PRINT("FSM transition CAPTURE -> CAPTURE_DONE\n");
    }
    else {
      state = OOO_ERROR;
      DEBUG_PRINT("FSM transition CAPTURE -> OOO_ERROR\n");  
    }
    return;
  }

  // if int0 is triggerd, proceed to TRIGGERED_0
  if (int0Triggered) {
    detachInterrupt(digitalPinToInterrupt(PIN_INT_0));  
    state = TRIGGERED_0;
    DEBUG_PRINT("FSM transition CAPTURE -> TRIGGERED_0\n");  
  }

  // if int1 is triggered, proceed to OOO_ERROR
  if (int1Triggered) {
    detachInterrupt(digitalPinToInterrupt(PIN_INT_0));
    detachInterrupt(digitalPinToInterrupt(PIN_INT_1));
    state = OOO_ERROR;
    DEBUG_PRINT("FSM transition CAPTURE -> OOO_ERROR\n");  
  }

  return;
}

/**
  * Handle the core loop logic for state TRIGGERED_0.
  * Display time since the first interrupt.
  * Transition to CAPTURE_DONE if interrupt 1 flag is set.
  */
void stateTRIGGERED() {
  // if int1 is triggerd, proceed to CAPTURE_DONE
  if (int1Triggered) {
    state = CAPTURE_DONE;
    DEBUG_PRINT("FSM transition TRIGGERED -> CAPTURE_DONE\n");
    detachInterrupt(digitalPinToInterrupt(PIN_INT_1));
    return;
  }

  // otherwise, compute time since first interrupt and display the value
  unsigned long delta = millis() - int0Time;
  setDisplayValue(delta);
}

/**
  * Handle the core loop logic for state CAPTURE_DONE.
  * Display the time delta between the two interrupts.
  */
void stateCAPTURE_DONE() {
  unsigned long delta = int1Time - int0Time;
  setDisplayValue(delta);
}

/**
  * Handle the core loop logic for state OOO_ERROR.
  * Display a 9 on all displays.
  */
void stateOOO_ERROR() {
  // show all 0's
  setDisplayDigit(0, 9);
  setDisplayDigit(1, 9);
  setDisplayDigit(2, 9);
  setDisplayDigit(3, 9);
}

/**
  * Handle the core loop logic for state GENERAL_ERROR.
  * Display the error code on the leftmost digit.
  */
void stateGENERAL_ERROR(uint8_t errCode) {
  blankDisplayDigit(0);
  blankDisplayDigit(1);
  blankDisplayDigit(2);
  setDisplayDigit(3, errCode);
}