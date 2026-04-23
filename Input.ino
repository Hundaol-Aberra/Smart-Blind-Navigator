/**
 * ============================================================
 *  Input.ino — User Input Module (Button + Microphone)
 * ============================================================
 *  Button behaviour:
 *    Short press  (<1 s)  → cycle through available destinations
 *    Long press   (≥1 s)  → confirm the currently selected destination
 *
 *  Microphone (simplified keyword detection):
 *    Uses amplitude threshold to detect clap patterns.
 *    1 clap  → cycle to next destination (same as short button press)
 *    2 claps → confirm destination       (same as long button press)
 *
 *  For production: replace the microphone section with a dedicated
 *  voice-recognition module (e.g. DFRobot Gravity or Elechouse V3).
 * ============================================================
 */

#include "Config.h"

// ──────────────────────────────────────────────
//  MODULE-PRIVATE STATE
// ──────────────────────────────────────────────

// Button
static bool     _lastButtonState  = HIGH;
static uint32_t _buttonPressTime  = 0;
static bool     _buttonHeld       = false;

// Microphone clap detection
static uint8_t  _clapCount        = 0;
static uint32_t _lastClapTime     = 0;
static bool     _inClap           = false;

// Microphone amplitude threshold (0-1023)
// Tune this value to suit your specific microphone module.
static const int MIC_THRESHOLD    = 600;

// ──────────────────────────────────────────────
//  INITIALISATION
// ──────────────────────────────────────────────

/**
 * @brief Configure GPIO pins for button and microphone.
 *        Call once from setup().
 */
void Input_Init() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // MIC_PIN is analog — no pinMode required for analogRead
  Serial.println(F("[INPUT] Button and microphone ready"));
}

// ──────────────────────────────────────────────
//  INTERNAL ACTION DISPATCHER
// ──────────────────────────────────────────────

/**
 * @brief Cycle the selected destination to the next option
 *        and announce it.
 */
static void Input_CycleDestination() {
  selectedDest = (selectedDest + 1) % NUM_LOCATIONS;
  Serial.print(F("[INPUT] Destination → "));
  Serial.println(locations[selectedDest].name);
  Audio_AnnounceDestination(selectedDest);
}

/**
 * @brief Confirm the currently highlighted destination
 *        and start navigation.
 */
static void Input_ConfirmDestination() {
  Serial.print(F("[INPUT] Confirmed: "));
  Serial.println(locations[selectedDest].name);
  Nav_Start(selectedDest);
}

// ──────────────────────────────────────────────
//  BUTTON HANDLER
// ──────────────────────────────────────────────

/**
 * @brief Non-blocking button state machine.
 *        Call every loop iteration.
 *
 *  State transitions:
 *    HIGH → LOW  : record press start time
 *    LOW  → HIGH : measure duration
 *                  < BUTTON_DEBOUNCE_MS  → ignore (bounce)
 *                  < BUTTON_LONG_PRESS_MS → short press (cycle)
 *                  ≥ BUTTON_LONG_PRESS_MS → long press (confirm)
 */
void Input_HandleButton() {
  bool currentState = digitalRead(BUTTON_PIN);
  uint32_t now      = millis();

  // Falling edge — button pressed down
  if (_lastButtonState == HIGH && currentState == LOW) {
    _buttonPressTime = now;
    _buttonHeld      = false;
  }

  // Rising edge — button released
  if (_lastButtonState == LOW && currentState == HIGH) {
    uint32_t duration = now - _buttonPressTime;

    if (duration < BUTTON_DEBOUNCE_MS) {
      // Ignore bounce
    } else if (duration < BUTTON_LONG_PRESS_MS) {
      Input_CycleDestination();
    } else {
      Input_ConfirmDestination();
    }
  }

  _lastButtonState = currentState;
}

// ──────────────────────────────────────────────
//  MICROPHONE / CLAP DETECTOR
// ──────────────────────────────────────────────

/**
 * @brief Amplitude-threshold clap detector.
 *        Call every loop iteration (rate-limited in main loop).
 *
 *  Algorithm:
 *  1. Sample MIC_PIN.
 *  2. If above threshold and not already "in a clap": rising edge detected.
 *     - If a previous clap happened within MIC_CLAP_WINDOW_MS → second clap.
 *     - Otherwise: first clap, record timestamp.
 *  3. If below threshold and was "in a clap": falling edge — clap ended.
 *  4. After MIC_CLAP_RESOLVE_MS of silence, resolve the pending count:
 *     - 1 clap  → cycle destination
 *     - 2 claps → confirm destination
 */
void Input_HandleMicrophone() {
  int      sample  = analogRead(MIC_PIN);
  bool     loud    = (sample > MIC_THRESHOLD);
  uint32_t now     = millis();

  // Detect onset of loud burst (rising edge)
  if (loud && !_inClap) {
    _inClap = true;

    if (_clapCount == 1 && (now - _lastClapTime) < MIC_CLAP_WINDOW_MS) {
      // Second clap within window
      _clapCount = 2;
    } else {
      // First clap (or restarted sequence)
      _clapCount    = 1;
      _lastClapTime = now;
    }
  }

  // Detect end of loud burst (falling edge)
  if (!loud && _inClap) {
    _inClap = false;
  }

  // Resolve pattern after silence window has passed
  if (_clapCount > 0 && (now - _lastClapTime) > MIC_CLAP_RESOLVE_MS) {
    if (_clapCount == 1) {
      Serial.println(F("[MIC] 1 clap → cycle destination"));
      Input_CycleDestination();
    } else if (_clapCount == 2) {
      Serial.println(F("[MIC] 2 claps → confirm destination"));
      Input_ConfirmDestination();
    }
    _clapCount = 0;   // Reset for next gesture
  }
}
