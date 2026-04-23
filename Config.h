/**
 * ============================================================
 *  Config.h — Global Configuration, Constants & Data Structures
 * ============================================================
 *  All pin definitions, timing constants, map size limits,
 *  shared structs, and extern declarations live here.
 *  Every other file includes this header.
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ──────────────────────────────────────────────
//  PIN DEFINITIONS
// ──────────────────────────────────────────────
#define RFID_SS_PIN       53    // RFID chip-select  (SPI)
#define RFID_RST_PIN       5    // RFID reset
#define SD_CS_PIN          4    // SD card chip-select (SPI)
#define BUTTON_PIN         2    // Push button (INPUT_PULLUP)
#define MIC_PIN           A0    // Microphone analog input
#define BUZZER_PIN         8    // Buzzer / speaker fallback

// ──────────────────────────────────────────────
//  MAP SIZE LIMITS
// ──────────────────────────────────────────────
#define MAX_LOCATIONS     10    // Maximum nodes in the map
#define MAX_EDGES         20    // Maximum directed edges
#define MAX_PATH_LEN      10    // Maximum steps in one route
#define NO_PATH           -1    // Sentinel: no route found

// ──────────────────────────────────────────────
//  AUDIO TRACK IDs
//  Files on SD card must be named 0001.mp3 … 0099.mp3
//  inside a folder called "01" (DFPlayer Mini convention).
// ──────────────────────────────────────────────
#define AUDIO_TURN_LEFT         1
#define AUDIO_TURN_RIGHT        2
#define AUDIO_GO_STRAIGHT       3
#define AUDIO_WRONG_PATH        4
#define AUDIO_ARRIVED           5
#define AUDIO_PLEASE_MOVE       6
#define AUDIO_SELECT_DEST       7
#define AUDIO_SCAN_DOOR         8
#define AUDIO_RECALCULATING     9
// Tracks 10–19 are per-destination announcements (10 + nodeId)
#define AUDIO_DEST_BASE        10

// ──────────────────────────────────────────────
//  TIMING CONSTANTS  (milliseconds)
// ──────────────────────────────────────────────
#define BUTTON_DEBOUNCE_MS      50
#define BUTTON_LONG_PRESS_MS  1000    // Hold duration to confirm destination
#define STATIONARY_WARN_MS    8000    // Warn after 8 s without movement
#define GUIDANCE_NORMAL_MS    5000    // Repeat instruction every 5 s (normal)
#define GUIDANCE_FREQUENT_MS  2500    // Repeat every 2.5 s (adaptive mode)
#define ACCEL_SAMPLE_MS        200    // Accelerometer poll rate
#define RFID_POLL_MS           300    // RFID scan poll rate
#define MIC_SAMPLE_MS          100    // Microphone sample rate
#define MIC_CLAP_WINDOW_MS    1000    // Window to detect second clap
#define MIC_CLAP_RESOLVE_MS   1200    // Silence duration to resolve pattern
#define WRONG_PATH_THRESHOLD     3    // Consecutive errors before frequent mode
#define MOTION_THRESHOLD     50000L   // Accelerometer delta² threshold

// ──────────────────────────────────────────────
//  DATA STRUCTURES
// ──────────────────────────────────────────────

/** A physical location / checkpoint in the building */
struct Location {
  uint8_t id;            // Unique node ID (0-based index)
  char    name[20];      // Human-readable label
  byte    rfidUID[4];    // 4-byte RFID tag UID
  uint8_t audioTrack;    // Announcement audio track number
};

/** A directed edge between two locations */
struct Edge {
  uint8_t from;            // Source node ID
  uint8_t to;              // Destination node ID
  uint8_t directionAudio;  // Audio track for the navigation instruction
};

/** State of the active navigation session */
struct NavSession {
  uint8_t currentNode;           // Last confirmed location (RFID scanned)
  uint8_t destinationNode;       // User-selected destination
  uint8_t path[MAX_PATH_LEN];    // BFS-computed route (array of node IDs)
  uint8_t pathLen;               // Total steps in route
  uint8_t pathStep;              // Index of the next expected node
  bool    active;                // True while a session is running
};

/** Adaptive guidance state */
struct AdaptiveState {
  uint8_t  wrongPathCount;       // Consecutive wrong-path scans
  bool     frequentMode;         // True when using shorter guidance interval
  uint32_t lastGuidanceTime;     // millis() of last instruction prompt
  uint32_t guidanceInterval;     // Current interval in ms
};

/** Motion state derived from accelerometer */
struct MotionState {
  bool    isMoving;              // True if user is currently walking
  uint32_t stationaryStart;     // millis() when movement stopped
  bool    warningPlayed;         // Prevents repeating "please move" too often
  int16_t prevAx, prevAy, prevAz; // Previous accelerometer readings
};

// ──────────────────────────────────────────────
//  EXTERN DECLARATIONS
//  Definitions live in MapData.ino; all other
//  files access them through these declarations.
// ──────────────────────────────────────────────
extern Location     locations[];
extern Edge         edges[];
extern const uint8_t NUM_LOCATIONS;
extern const uint8_t NUM_EDGES;

// Shared state — defined in SmartBlindNavigator.ino
extern NavSession    nav;
extern AdaptiveState adaptive;
extern MotionState   motion;
extern uint8_t       selectedDest;

#endif // CONFIG_H
