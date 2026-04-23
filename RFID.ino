/**
 * ============================================================
 *  RFID.ino — RFID Reader Module (MFRC522)
 * ============================================================
 *  Handles all RFID hardware interaction:
 *    - Initialisation of the MFRC522 over SPI
 *    - Non-blocking polling for new cards
 *    - UID-to-location matching
 *
 *  Wiring (SPI on Arduino Mega):
 *    MFRC522 SDA  → pin 53  (RFID_SS_PIN)
 *    MFRC522 SCK  → pin 52
 *    MFRC522 MOSI → pin 51
 *    MFRC522 MISO → pin 50
 *    MFRC522 RST  → pin  5  (RFID_RST_PIN)
 *    MFRC522 3.3V → 3.3 V
 *    MFRC522 GND  → GND
 * ============================================================
 */

#include "Config.h"
#include <SPI.h>
#include <MFRC522.h>

// ── MFRC522 object ──────────────────────────────────────────
static MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// ──────────────────────────────────────────────
//  INITIALISATION
// ──────────────────────────────────────────────

/**
 * @brief Initialise the SPI bus and MFRC522 reader.
 *        Must be called once from setup().
 */
void RFID_Init() {
  SPI.begin();
  rfid.PCD_Init();

  // Print firmware version for diagnostics
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("[RFID] Firmware v"));
  Serial.println(version, HEX);
  Serial.println(F("[RFID] Reader ready"));
}

// ──────────────────────────────────────────────
//  INTERNAL HELPERS
// ──────────────────────────────────────────────

/**
 * @brief Compare two 4-byte UID arrays for equality.
 *
 * @param a   First UID buffer
 * @param b   Second UID buffer
 * @return    true if all 4 bytes match
 */
static bool RFID_UIDMatch(byte* a, byte* b) {
  for (uint8_t i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// ──────────────────────────────────────────────
//  PUBLIC API
// ──────────────────────────────────────────────

/**
 * @brief Non-blocking scan for an RFID card.
 *        Matches the scanned UID against the location table.
 *
 * @return  Index of the matched location (0 … NUM_LOCATIONS-1),
 *          or -1 if no card present or UID is unrecognised.
 */
int8_t RFID_Scan() {
  // Check if a new card is in the field
  if (!rfid.PICC_IsNewCardPresent()) return -1;
  if (!rfid.PICC_ReadCardSerial())   return -1;

  byte* uid = rfid.uid.uidByte;

  // Debug: print raw UID to serial
  Serial.print(F("[RFID] UID:"));
  for (uint8_t i = 0; i < rfid.uid.size; i++) {
    Serial.print(' ');
    if (uid[i] < 0x10) Serial.print('0');
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  // Match against known location UIDs
  for (uint8_t i = 0; i < NUM_LOCATIONS; i++) {
    if (RFID_UIDMatch(uid, locations[i].rfidUID)) {
      rfid.PICC_HaltA();         // Stop reading this card
      rfid.PCD_StopCrypto1();    // Stop encrypted comms
      return (int8_t)i;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  Serial.println(F("[RFID] Unknown tag — not in map"));
  return -1;
}
