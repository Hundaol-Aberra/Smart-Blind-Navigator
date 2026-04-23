/**
 * ============================================================
 *  Accelerometer.ino — Motion Detection Module (MPU-6050)
 * ============================================================
 *  Detects whether the user is walking or stationary by
 *  comparing successive accelerometer readings.
 *
 *  Algorithm:
 *    - Sample ax, ay, az every ACCEL_SAMPLE_MS milliseconds.
 *    - Compute the squared magnitude of the change vector
 *      (Δax² + Δay² + Δaz²) — avoids expensive sqrt().
 *    - If above MOTION_THRESHOLD → user is moving.
 *    - If below threshold → user is stationary; record start time.
 *    - Motion state is written into the global `motion` struct
 *      so Navigation.ino can issue the "please move" warning.
 *
 *  Wiring (I2C on Arduino Mega):
 *    MPU-6050 SDA → pin 20
 *    MPU-6050 SCL → pin 21
 *    MPU-6050 VCC → 3.3 V or 5 V (check your module)
 *    MPU-6050 GND → GND
 *    MPU-6050 AD0 → GND  (I2C address 0x68)
 * ============================================================
 */

#include "Config.h"
#include <Wire.h>
#include <MPU6050.h>

// ── MPU-6050 object ─────────────────────────────────────────
static MPU6050 mpu;

// ── Connection flag ─────────────────────────────────────────
static bool _mpuReady = false;

// ──────────────────────────────────────────────
//  INITIALISATION
// ──────────────────────────────────────────────

/**
 * @brief Initialise the I2C bus and MPU-6050.
 *        Must be called once from setup().
 */
void Accel_Init() {
  Wire.begin();
  mpu.initialize();

  _mpuReady = mpu.testConnection();

  if (_mpuReady) {
    Serial.println(F("[ACCEL] MPU-6050 connected OK"));
    // Optional: configure sensitivity (default ±2 g is fine)
    // mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  } else {
    Serial.println(F("[ACCEL] MPU-6050 NOT found — motion detection disabled"));
  }

  // Mark user as initially stationary
  motion.isMoving        = false;
  motion.stationaryStart = millis();
  motion.warningPlayed   = false;
  motion.prevAx          = 0;
  motion.prevAy          = 0;
  motion.prevAz          = 0;
}

// ──────────────────────────────────────────────
//  MAIN HANDLER  (call every ACCEL_SAMPLE_MS)
// ──────────────────────────────────────────────

/**
 * @brief Read accelerometer and update the global `motion` struct.
 *        Non-blocking — caller is responsible for rate-limiting.
 */
void Accel_Handle() {
  if (!_mpuReady) return;

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Squared-delta magnitude (no sqrt needed for threshold comparison)
  int32_t dx    = (int32_t)ax - motion.prevAx;
  int32_t dy    = (int32_t)ay - motion.prevAy;
  int32_t dz    = (int32_t)az - motion.prevAz;
  int32_t delta = (dx * dx) + (dy * dy) + (dz * dz);

  bool nowMoving = (delta > MOTION_THRESHOLD);

  // Only act on state transitions to avoid noisy repeated updates
  if (nowMoving != motion.isMoving) {
    motion.isMoving = nowMoving;

    if (nowMoving) {
      // User started moving — clear stationary tracking
      motion.warningPlayed = false;
      Serial.println(F("[ACCEL] User moving"));
    } else {
      // User stopped — begin stationary timer
      motion.stationaryStart = millis();
      motion.warningPlayed   = false;
      Serial.println(F("[ACCEL] User stationary"));
    }
  }

  // Store readings for next comparison
  motion.prevAx = ax;
  motion.prevAy = ay;
  motion.prevAz = az;
}
