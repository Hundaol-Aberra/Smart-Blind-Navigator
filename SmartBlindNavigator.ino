/**
 * ============================================================
 *  SmartBlindNavigator.ino — MAIN FILE
 * ============================================================
 *  Smart Indoor Navigation Aid for the Visually Impaired
 *  Microcontroller: Arduino Mega (ATmega2560)
 *
 *  FILE STRUCTURE (all files in the same sketch folder):
 *  ┌─────────────────────────────┬──────────────────────────────────────┐
 *  │ File                        │ Responsibility                       │
 *  ├─────────────────────────────┼──────────────────────────────────────┤
 *  │ SmartBlindNavigator.ino     │ ← YOU ARE HERE                       │
 *  │                             │   setup(), loop(), global state      │
 *  │ Config.h                    │   Pin defs, constants, structs,      │
 *  │                             │   extern declarations                │
 *  │ MapData.ino                 │   locations[] and edges[] tables     │
 *  │                             │   (edit here to add rooms/paths)     │
 *  │ Audio.ino                   │   DFPlayer Mini driver, play()       │
 *  │ RFID.ino                    │   MFRC522 driver, tag scanning       │
 *  │ Navigation.ino              │   BFS pathfinding, session control   │
 *  │                             │   guidance prompts, arrival logic    │
 *  │ Input.ino                   │   Push button + microphone/clap      │
 *  │ Accelerometer.ino           │   MPU-6050 motion detection          │
 *  └─────────────────────────────┴──────────────────────────────────────┘
 *
 *  HOW THE ARDUINO IDE HANDLES MULTIPLE .ino FILES:
 *    The IDE automatically concatenates every .ino file in the
 *    sketch folder before compiling, using this file as the root.
 *    You do NOT need #include for the other .ino files — just
 *    keep them all in the same folder.
 *    Config.h IS #included explicitly because it is a header.
 *
 *  REQUIRED LIBRARIES (install via Library Manager):
 *    - MFRC522          by miguelbalboa
 *    - MPU6050          by Electronic Cats
 *    - SD               (built-in)
 *    - Wire             (built-in)
 *    - SPI              (built-in)
 *
 *  HARDWARE CONNECTIONS SUMMARY:
 *    MFRC522 RFID  → SPI  (SS=53, RST=5, SCK=52, MOSI=51, MISO=50)
 *    MPU-6050      → I2C  (SDA=20, SCL=21)
 *    DFPlayer Mini → UART (TX1=18 → DFP-RX, RX1=19 ← DFP-TX)
 *    SD card       → SPI  (CS=4)
 *    Push button   → pin 2 + GND  (INPUT_PULLUP)
 *    Microphone    → A0
 *    Buzzer        → pin 8
 * ============================================================
 */

#include "Config.h"

// ──────────────────────────────────────────────
//  GLOBAL STATE  (declared here, extern'd in Config.h)
// ──────────────────────────────────────────────

NavSession nav = {
  /* currentNode     */ 0,
  /* destinationNode */ 0,
  /* path            */ {},
  /* pathLen         */ 0,
  /* pathStep        */ 0,
  /* active          */ false
};

AdaptiveState adaptive = {
  /* wrongPathCount   */ 0,
  /* frequentMode     */ false,
  /* lastGuidanceTime */ 0,
  /* guidanceInterval */ GUIDANCE_NORMAL_MS
};

MotionState motion = {
  /* isMoving         */ false,
  /* stationaryStart  */ 0,
  /* warningPlayed    */ false,
  /* prevAx/Ay/Az     */ 0, 0, 0
};

uint8_t selectedDest = 0;  // Currently highlighted destination (button / mic)

// ──────────────────────────────────────────────
//  POLL TIMERS  (millis-based, non-blocking)
// ──────────────────────────────────────────────
static uint32_t _lastRfidTime  = 0;
static uint32_t _lastAccelTime = 0;
static uint32_t _lastMicTime   = 0;

// ──────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────

void setup() {
  // Debug serial (USB)
  Serial.begin(9600);
  while (!Serial && millis() < 3000);   // Wait up to 3 s for Serial on Mega
  Serial.println(F("===================================="));
  Serial.println(F("  Smart Blind Navigator — Starting  "));
  Serial.println(F("===================================="));

  // Initialise buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialise each module
  Audio_Init();          // Audio.ino        — DFPlayer Mini via Serial1
  RFID_Init();           // RFID.ino         — MFRC522 via SPI
  Accel_Init();          // Accelerometer.ino— MPU-6050 via I2C
  Input_Init();          // Input.ino        — button + mic GPIO

  // Set initial stationary start time after everything is ready
  motion.stationaryStart = millis();
  adaptive.lastGuidanceTime = millis();

  Serial.println(F("[MAIN] All modules initialised"));
  Serial.println(F("[MAIN] Press button or clap to select destination"));

  // Prompt user to select a destination
  Audio_Play(AUDIO_SELECT_DEST);
}

// ──────────────────────────────────────────────
//  MAIN LOOP  — fully non-blocking
// ──────────────────────────────────────────────

void loop() {
  uint32_t now = millis();

  // ── 1. Button input (every iteration — debounce handled inside) ──
  Input_HandleButton();

  // ── 2. RFID scan (rate-limited) ─────────────────────────────────
  if (now - _lastRfidTime >= RFID_POLL_MS) {
    _lastRfidTime = now;

    int8_t scannedNode = RFID_Scan();      // RFID.ino
    if (scannedNode >= 0) {
      Nav_OnRFIDScanned((uint8_t)scannedNode);  // Navigation.ino
    }
  }

  // ── 3. Accelerometer / motion detection (rate-limited) ──────────
  if (now - _lastAccelTime >= ACCEL_SAMPLE_MS) {
    _lastAccelTime = now;
    Accel_Handle();                        // Accelerometer.ino
  }

  // ── 4. Microphone clap detection (rate-limited) ─────────────────
  if (now - _lastMicTime >= MIC_SAMPLE_MS) {
    _lastMicTime = now;
    Input_HandleMicrophone();             // Input.ino
  }

  // ── 5. Periodic guidance prompt (only while nav active) ─────────
  Nav_HandleGuidance();                  // Navigation.ino

  // ── 6. Stationary-too-long warning (only while nav active) ──────
  Nav_HandleMotionWarning();             // Navigation.ino
}
