/**
 * ============================================================
 *  MapData.ino — Building Map: Locations & Edges
 * ============================================================
 *  Edit ONLY this file to add or remove locations and paths.
 *
 *  HOW TO ADD A NEW LOCATION:
 *  1. Scan the physical RFID tag (use a UID-dump sketch) and
 *     note its 4-byte UID.
 *  2. Add a new Location entry in locations[] below, using the
 *     next sequential ID.
 *  3. Add Edge entries connecting it to neighbouring nodes,
 *     choosing the appropriate direction audio constant.
 *  4. Bump NUM_LOCATIONS and NUM_EDGES accordingly.
 *  5. Record an audio file (e.g. 0016.mp3) on the SD card for
 *     the location announcement and update audioTrack field.
 *
 *  DIRECTION AUDIO OPTIONS:
 *    AUDIO_TURN_LEFT   (1)
 *    AUDIO_TURN_RIGHT  (2)
 *    AUDIO_GO_STRAIGHT (3)
 * ============================================================
 */

#include "Config.h"

// ── Number of entries — update when you add rows ────────────
const uint8_t NUM_LOCATIONS = 6;
const uint8_t NUM_EDGES     = 8;

// ── Location Table ───────────────────────────────────────────
//   { id,  "Name",          { UID byte 0-3 },          audioTrack }
Location locations[MAX_LOCATIONS] = {
  {  0,  "Entrance",    { 0xA1, 0xB2, 0xC3, 0xD4 },   10 },
  {  1,  "Room A",      { 0x11, 0x22, 0x33, 0x44 },   11 },
  {  2,  "Room B",      { 0x55, 0x66, 0x77, 0x88 },   12 },
  {  3,  "Cafeteria",   { 0x99, 0xAA, 0xBB, 0xCC },   13 },
  {  4,  "Bathroom",    { 0xDD, 0xEE, 0xFF, 0x00 },   14 },
  {  5,  "Exit",        { 0x12, 0x34, 0x56, 0x78 },   15 },
  // Add more locations here …
};

// ── Edge Table (directed connections) ────────────────────────
//   { from, to, directionAudio }
Edge edges[MAX_EDGES] = {
  {  0,  1,  AUDIO_GO_STRAIGHT  },   // Entrance  → Room A
  {  0,  3,  AUDIO_TURN_RIGHT   },   // Entrance  → Cafeteria
  {  1,  2,  AUDIO_TURN_LEFT    },   // Room A    → Room B
  {  1,  0,  AUDIO_TURN_RIGHT   },   // Room A    → Entrance (back)
  {  2,  4,  AUDIO_GO_STRAIGHT  },   // Room B    → Bathroom
  {  3,  5,  AUDIO_GO_STRAIGHT  },   // Cafeteria → Exit
  {  3,  0,  AUDIO_TURN_LEFT    },   // Cafeteria → Entrance (back)
  {  4,  5,  AUDIO_TURN_RIGHT   },   // Bathroom  → Exit
  // Add more edges here …
};
