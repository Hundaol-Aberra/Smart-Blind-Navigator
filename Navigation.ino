/**
 * ============================================================
 *  Navigation.ino — Pathfinding & Navigation Logic
 * ============================================================
 *  Implements:
 *    - BFS (Breadth-First Search) shortest-hop pathfinding
 *    - Session start / step advance / wrong-path / arrival
 *    - Adaptive guidance interval control
 *    - Periodic guidance prompt
 *    - Motion-stationary warning
 * ============================================================
 */

#include "Config.h"

// ──────────────────────────────────────────────
//  INTERNAL HELPERS
// ──────────────────────────────────────────────

/**
 * @brief Look up the direction audio track for an edge.
 *
 * @param from  Source node ID
 * @param to    Destination node ID
 * @return      Audio track number, or -1 if edge not found
 */
static int8_t Nav_GetEdgeAudio(uint8_t from, uint8_t to) {
  for (uint8_t e = 0; e < NUM_EDGES; e++) {
    if (edges[e].from == from && edges[e].to == to) {
      return (int8_t)edges[e].directionAudio;
    }
  }
  return -1;
}

// ──────────────────────────────────────────────
//  BFS PATHFINDING
// ──────────────────────────────────────────────

/**
 * @brief Find the shortest path (fewest hops) from src to dst
 *        using Breadth-First Search on the edge table.
 *
 *        Uses only fixed-size stack arrays — no dynamic memory.
 *
 * @param src      Starting node ID
 * @param dst      Target node ID
 * @param outPath  Output array to fill with node IDs (excluding src)
 * @param outLen   Output: number of steps written to outPath
 * @return         true if a path was found, false otherwise
 */
bool Nav_BFS(uint8_t src, uint8_t dst,
             uint8_t* outPath, uint8_t& outLen) {

  int8_t  parent[MAX_LOCATIONS];
  bool   visited[MAX_LOCATIONS];

  for (uint8_t i = 0; i < NUM_LOCATIONS; i++) {
    parent[i]  = -1;
    visited[i] = false;
  }

  // Simple FIFO queue using a circular fixed-size array
  uint8_t queue[MAX_LOCATIONS];
  uint8_t qHead = 0, qTail = 0;

  visited[src]       = true;
  queue[qTail++]     = src;

  bool found = false;

  while (qHead != qTail && !found) {
    uint8_t node = queue[qHead++];
    if (qHead >= MAX_LOCATIONS) qHead = 0;  // wrap

    for (uint8_t e = 0; e < NUM_EDGES; e++) {
      if (edges[e].from != node) continue;

      uint8_t next = edges[e].to;
      if (visited[next]) continue;

      visited[next] = true;
      parent[next]  = (int8_t)node;

      queue[qTail] = next;
      qTail++;
      if (qTail >= MAX_LOCATIONS) qTail = 0; // wrap

      if (next == dst) {
        found = true;
        break;
      }
    }
  }

  if (!found) {
    outLen = 0;
    return false;
  }

  // Reconstruct path by following parent[] backwards from dst
  uint8_t temp[MAX_PATH_LEN];
  uint8_t len = 0;
  uint8_t cur = dst;

  while (cur != src && len < MAX_PATH_LEN) {
    temp[len++] = cur;
    cur = (uint8_t)parent[cur];
  }

  // Reverse into caller's output array (src excluded, dst included)
  outLen = len;
  for (uint8_t i = 0; i < len; i++) {
    outPath[i] = temp[len - 1 - i];
  }

  return true;
}

// ──────────────────────────────────────────────
//  SESSION CONTROL
// ──────────────────────────────────────────────

/**
 * @brief Start a new navigation session toward the given destination.
 *        Runs BFS, stores the route, and plays the first instruction.
 *
 * @param dest  Destination node ID
 */
void Nav_Start(uint8_t dest) {
  if (dest == nav.currentNode) {
    Serial.println(F("[NAV] Already at destination"));
    Audio_Play(AUDIO_ARRIVED);
    return;
  }

  bool found = Nav_BFS(nav.currentNode, dest, nav.path, nav.pathLen);

  if (!found) {
    Serial.println(F("[NAV] ERROR: No path found to destination"));
    return;
  }

  // Populate session
  nav.destinationNode       = dest;
  nav.pathStep              = 0;
  nav.active                = true;

  // Reset adaptive state
  adaptive.wrongPathCount   = 0;
  adaptive.frequentMode     = false;
  adaptive.guidanceInterval = GUIDANCE_NORMAL_MS;
  adaptive.lastGuidanceTime = millis();

  Serial.print(F("[NAV] Route to: "));
  Serial.println(locations[dest].name);
  Serial.print(F("[NAV] Steps: "));
  Serial.println(nav.pathLen);

  // Print the full route for debug
  Serial.print(F("[NAV] Path: "));
  Serial.print(locations[nav.currentNode].name);
  for (uint8_t i = 0; i < nav.pathLen; i++) {
    Serial.print(F(" → "));
    Serial.print(locations[nav.path[i]].name);
  }
  Serial.println();

  // Play first direction immediately
  Nav_PlayCurrentInstruction();
  Audio_Play(AUDIO_SCAN_DOOR);
}

/**
 * @brief Called when the user scans the correct next checkpoint.
 *        Advances the path step or triggers arrival.
 */
void Nav_Advance() {
  nav.pathStep++;

  if (nav.pathStep >= nav.pathLen) {
    Nav_OnArrived();
  } else {
    Serial.print(F("[NAV] ✓ Correct scan → next: "));
    Serial.println(locations[nav.path[nav.pathStep]].name);

    // Reset timer so next instruction plays promptly
    adaptive.lastGuidanceTime = 0;
    Audio_Play(AUDIO_SCAN_DOOR);
  }
}

/**
 * @brief Called when the user scans an unexpected node.
 *        Increments error counter, activates adaptive mode if
 *        threshold is exceeded, and recalculates the route.
 */
void Nav_WrongPath() {
  adaptive.wrongPathCount++;

  Serial.print(F("[NAV] ✗ Wrong path! Error count: "));
  Serial.println(adaptive.wrongPathCount);

  Audio_Play(AUDIO_WRONG_PATH);

  // Adaptive logic: too many errors → shorten guidance interval
  if (!adaptive.frequentMode &&
      adaptive.wrongPathCount >= WRONG_PATH_THRESHOLD) {
    adaptive.frequentMode     = true;
    adaptive.guidanceInterval = GUIDANCE_FREQUENT_MS;
    Serial.println(F("[NAV] Frequent guidance mode activated"));
  }

  // Recalculate from current (wrong) node
  Audio_Play(AUDIO_RECALCULATING);

  bool found = Nav_BFS(nav.currentNode, nav.destinationNode,
                       nav.path, nav.pathLen);
  if (found) {
    nav.pathStep = 0;
    Serial.println(F("[NAV] Route recalculated successfully"));
    Nav_PlayCurrentInstruction();
  } else {
    Serial.println(F("[NAV] Cannot recalculate — no route available"));
    nav.active = false;
  }
}

/**
 * @brief Called when the user reaches the destination node.
 *        Resets the session and plays the success message.
 */
void Nav_OnArrived() {
  Audio_Play(AUDIO_ARRIVED);
  Serial.println(F("[NAV] *** ARRIVED at destination ***"));

  nav.active                = false;
  nav.pathLen               = 0;
  nav.pathStep              = 0;
  adaptive.wrongPathCount   = 0;
  adaptive.frequentMode     = false;
  adaptive.guidanceInterval = GUIDANCE_NORMAL_MS;
}

// ──────────────────────────────────────────────
//  PERIODIC HANDLERS  (called from main loop)
// ──────────────────────────────────────────────

/**
 * @brief Play the current step's direction audio if enough
 *        time has elapsed since the last prompt.
 *        Must be called every loop iteration while nav.active.
 */
void Nav_HandleGuidance() {
  if (!nav.active) return;

  uint32_t now = millis();
  if (now - adaptive.lastGuidanceTime < adaptive.guidanceInterval) return;

  adaptive.lastGuidanceTime = now;
  Nav_PlayCurrentInstruction();
}

/**
 * @brief Warn the user if they have been stationary too long.
 *        Must be called every loop iteration while nav.active.
 */
void Nav_HandleMotionWarning() {
  if (!nav.active)           return;
  if (motion.isMoving)       return;
  if (motion.warningPlayed)  return;

  uint32_t stationaryDuration = millis() - motion.stationaryStart;

  if (stationaryDuration >= STATIONARY_WARN_MS) {
    Audio_Play(AUDIO_PLEASE_MOVE);
    motion.warningPlayed = true;   // Only warn once per stationary event
    Serial.println(F("[NAV] Stationary warning issued"));
  }
}

/**
 * @brief Play the direction audio for the current path step.
 */
void Nav_PlayCurrentInstruction() {
  if (nav.pathStep >= nav.pathLen) return;

  uint8_t from = (nav.pathStep == 0)
                   ? nav.currentNode
                   : nav.path[nav.pathStep - 1];
  uint8_t to   = nav.path[nav.pathStep];

  int8_t dir = Nav_GetEdgeAudio(from, to);

  if (dir >= 0) {
    Audio_Play((uint8_t)dir);
  }

  Serial.print(F("[NAV] Instruction: "));
  Serial.print(locations[from].name);
  Serial.print(F(" → "));
  Serial.println(locations[to].name);
}

// ──────────────────────────────────────────────
//  RFID EVENT HANDLER
//  Called by the main loop when a tag is scanned.
// ──────────────────────────────────────────────

/**
 * @brief Process a freshly scanned RFID node.
 *        Updates current location, then drives the navigation FSM.
 *
 * @param scannedNode  Location index returned by RFID_Scan()
 */
void Nav_OnRFIDScanned(uint8_t scannedNode) {
  // Always update where we are
  nav.currentNode = scannedNode;
  Audio_AnnounceLocation(scannedNode);

  if (!nav.active) return;

  uint8_t expectedNode = nav.path[nav.pathStep];

  if (scannedNode == expectedNode) {
    // ── Correct checkpoint ─────────────────────
    // If user was in frequent mode and now correct: restore normal
    if (adaptive.frequentMode) {
      adaptive.wrongPathCount   = 0;
      adaptive.frequentMode     = false;
      adaptive.guidanceInterval = GUIDANCE_NORMAL_MS;
      Serial.println(F("[NAV] Back on track — normal guidance restored"));
    }
    Nav_Advance();

  } else if (scannedNode == nav.path[nav.pathLen - 1]) {
    // ── Reached final destination via shortcut ─
    Nav_OnArrived();

  } else {
    // ── Wrong node ────────────────────────────
    Nav_WrongPath();
  }
}
