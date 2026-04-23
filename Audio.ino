/**
 * ============================================================
 *  Audio.ino — Audio Output Module
 * ============================================================
 *  Handles all sound output via DFPlayer Mini (Serial1).
 *  Falls back to Serial monitor labels + buzzer beep when
 *  the DFPlayer is not connected (simulation / debug mode).
 *
 *  DFPlayer Mini wiring:
 *    TX1 (Arduino pin 18) → RX  (DFPlayer pin 2)
 *    RX1 (Arduino pin 19) → TX  (DFPlayer pin 1)
 *    GND                  → GND
 *    5V                   → VCC
 *    SPK1/SPK2            → 8-ohm speaker or earpiece
 *
 *  SD card file naming (inside folder "01" on the card):
 *    0001.mp3  "Turn left"
 *    0002.mp3  "Turn right"
 *    0003.mp3  "Go straight"
 *    0004.mp3  "Wrong path, please stop"
 *    0005.mp3  "You have arrived at your destination"
 *    0006.mp3  "Please continue moving"
 *    0007.mp3  "Please select your destination"
 *    0008.mp3  "Please scan the RFID tag on the door"
 *    0009.mp3  "Recalculating your route"
 *    0010.mp3  "Entrance"          ← location name tracks
 *    0011.mp3  "Room A"
 *    0012.mp3  "Room B"
 *    … and so on for each location
 * ============================================================
 */

#include "Config.h"

// ──────────────────────────────────────────────
//  AUDIO INITIALISATION
// ──────────────────────────────────────────────

/**
 * @brief Initialise Serial1 for the DFPlayer Mini and
 *        send a reset command to put it in a known state.
 */
void Audio_Init() {
  Serial1.begin(9600);
  delay(1000);              // DFPlayer needs ~1 s after power-on
  Audio_SendCommand(0x0C, 0, 0);  // Reset
  delay(500);
  Audio_SendCommand(0x06, 0, 20); // Set volume (0–30)
  Serial.println(F("[AUDIO] DFPlayer initialised (vol=20)"));
}

// ──────────────────────────────────────────────
//  LOW-LEVEL DFPlayer COMMAND
// ──────────────────────────────────────────────

/**
 * @brief Send a raw 10-byte DFPlayer Mini command frame.
 *
 * Frame format:
 *   7E FF 06 <cmd> 00 <paramH> <paramL> <csumH> <csumL> EF
 *
 * @param cmd    Command byte (e.g. 0x03 = play by number)
 * @param paramH High byte of parameter
 * @param paramL Low byte of parameter
 */
void Audio_SendCommand(uint8_t cmd, uint8_t paramH, uint8_t paramL) {
  uint8_t frame[10];
  frame[0] = 0x7E;
  frame[1] = 0xFF;
  frame[2] = 0x06;
  frame[3] = cmd;
  frame[4] = 0x00;          // Feedback byte (0 = no feedback)
  frame[5] = paramH;
  frame[6] = paramL;

  // Checksum = 2's complement of sum of bytes 1..6
  int16_t sum = 0;
  for (uint8_t i = 1; i <= 6; i++) sum += frame[i];
  sum = -sum;
  frame[7] = (uint8_t)(sum >> 8);
  frame[8] = (uint8_t)(sum & 0xFF);
  frame[9] = 0xEF;

  Serial1.write(frame, 10);
}

// ──────────────────────────────────────────────
//  PUBLIC API
// ──────────────────────────────────────────────

/**
 * @brief Play a numbered audio track.
 *        Also prints a label to Serial for simulation/debug.
 *
 * @param track  Track number (1-based, matches filename 000N.mp3)
 */
void Audio_Play(uint8_t track) {
  // Send play-specific-track command (0x03)
  Audio_SendCommand(0x03, 0, track);

  // Buzzer beep as a supplementary tactile-like cue
  tone(BUZZER_PIN, 880, 80);

  // Serial simulation label
  Serial.print(F("[AUDIO] Track "));
  Serial.print(track);
  Serial.print(F(" → "));
  Audio_PrintLabel(track);
}

/**
 * @brief Announce the name of a location node.
 * @param nodeId  Index into locations[]
 */
void Audio_AnnounceLocation(uint8_t nodeId) {
  Serial.print(F("[LOC] At: "));
  Serial.println(locations[nodeId].name);
  Audio_Play(locations[nodeId].audioTrack);
}

/**
 * @brief Announce the selected destination name.
 * @param nodeId  Index into locations[]
 */
void Audio_AnnounceDestination(uint8_t nodeId) {
  Serial.print(F("[DEST] Heading to: "));
  Serial.println(locations[nodeId].name);
  Audio_Play(AUDIO_DEST_BASE + nodeId);
}

// ──────────────────────────────────────────────
//  DEBUG / SIMULATION LABEL PRINTER
// ──────────────────────────────────────────────

/**
 * @brief Print a human-readable label for a given track number.
 *        Used only for Serial monitor simulation.
 */
void Audio_PrintLabel(uint8_t track) {
  switch (track) {
    case AUDIO_TURN_LEFT:      Serial.println(F("Turn left"));                    break;
    case AUDIO_TURN_RIGHT:     Serial.println(F("Turn right"));                   break;
    case AUDIO_GO_STRAIGHT:    Serial.println(F("Go straight"));                  break;
    case AUDIO_WRONG_PATH:     Serial.println(F("Wrong path! Please stop."));     break;
    case AUDIO_ARRIVED:        Serial.println(F("You have arrived!"));             break;
    case AUDIO_PLEASE_MOVE:    Serial.println(F("Please continue moving"));       break;
    case AUDIO_SELECT_DEST:    Serial.println(F("Please select your destination"));break;
    case AUDIO_SCAN_DOOR:      Serial.println(F("Scan the RFID tag on the door"));break;
    case AUDIO_RECALCULATING:  Serial.println(F("Recalculating your route"));     break;
    default:
      if (track >= AUDIO_DEST_BASE &&
          track <  AUDIO_DEST_BASE + NUM_LOCATIONS) {
        Serial.println(locations[track - AUDIO_DEST_BASE].name);
      } else {
        Serial.println(F("(unknown track)"));
      }
      break;
  }
}
