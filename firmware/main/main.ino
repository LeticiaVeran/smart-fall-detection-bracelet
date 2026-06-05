// ══════════════════════════════════════════════════════════════
// main.ino
// Smart Bracelet for Elderly Monitoring and Safety
// ──────────────────────────────────────────────────────────────
// Course: Integrative Project I — 2026.1
// Federal University of Santa Catarina (UFSC) — Araranguá Campus
// ──────────────────────────────────────────────────────────────
// Hardware:
//   - ESP32-WROOM-32
//   - MPU6050 sensor (GY-521) — GPIO 21 (SDA), GPIO 22 (SCL)
//   - Active 5V buzzer        — GPIO 19 (inverted logic)
//   - Push button             — GPIO 18 (INPUT_PULLUP)
// ──────────────────────────────────────────────────────────────
// Algorithm based on:
//   Bourke & Lyons (2008) — Medical Engineering & Physics
//   Al-Dahan et al. (2016) — ICOST, Springer
// ══════════════════════════════════════════════════════════════

#include <Wire.h>         // I2C with the MPU6050 (native to the ESP32)
#include <WiFi.h>         // WiFi (native to the ESP32)
#include <PubSubClient.h> // MQTT client — install via Library Manager

// ── CREDENTIALS ───────────────────────────────────────────────
// Do not edit here — copy config.h.example to config.h and edit there
#include "config.h"

// ── WIFI AND MQTT OBJECTS ─────────────────────────────────────
WiFiClient   espClient;
PubSubClient mqtt(espClient);

// ── ALGORITHM PARAMETERS (±4g scale — sensitivity 8,192 LSB/g) ──
// All values were calibrated with real sensor data.
// At rest the magnitude is ~8,192 (equivalent to 1g of gravity).

// Phase 1 — Free fall
// Real data: free fall recorded 1,691–2,833 LSB
const float FREE_FALL_THRESHOLD = 2500;

// Phase 2 — Impact
// Real data: impact recorded 10,047–15,994 LSB
const float IMPACT_THRESHOLD    = 9500;

// Phase 3 — Rest
// MUST be greater than 8,192 (rest value) to work
const float REST_THRESHOLD      = 9500;

// Gyroscope — confidence bonus (rotation during a real fall)
// ~152°/s on the ±500°/s scale (65.5 LSB/°/s × 152 ≈ 10,000)
const float GYRO_THRESHOLD      = 10000;

// Jerk — sharp change in acceleration that signals the start of a fall
// Computed as |magnitude(t) - magnitude(t-1)| / time
const float JERK_THRESHOLD      = 35000;

// Maximum time window between free fall and impact (ms)
const int   TIME_WINDOW         = 1000;

// Minimum rest time to confirm a real fall (ms)
// 3s distinguishes a real fall (body still) from a gesture (body keeps moving)
const int   REST_DURATION       = 3000;

// Silence after an alert — avoids duplicate alerts (ms)
const int   POST_FALL_LOCKOUT   = 4000;

// Initial readings to ignore — the sensor needs to stabilize
const int   WARMUP_READINGS     = 15;

// Consecutive I2C failures before reinitializing the sensor
// The MPU6050 may enter sleep mode after current spikes
// 5 failures × 50ms = ~250ms without response → reinitialize
const int   MAX_FAILURES        = 5;

// ── HARDWARE DEFINITIONS ──────────────────────────────────────
#define MPU_ADDR    0x68  // I2C address with AD0 tied to GND
#define BUTTON_PIN  18    // INPUT_PULLUP: HIGH=released, LOW=pressed
#define BUZZER_PIN  19    // Inverted logic: LOW=on, HIGH=off

// ── ALGORITHM STATE VARIABLES ─────────────────────────────────
bool freeFallDetected      = false; // true when phase 1 is active
bool rotationDetectedInFall = false; // true if gyroscope > threshold in phase 1
bool impactDetected        = false; // true when phase 2 is active
int  warmupCount           = 0;     // counts warmup readings
int  i2cFailureCount       = 0;     // counts consecutive read failures

// Timestamps (unsigned long to support millis() indefinitely)
unsigned long freeFallTime  = 0; // moment phase 1 was detected
unsigned long impactTime    = 0; // moment phase 2 was detected
unsigned long lastFallTime  = 0; // moment of the last alert sent
unsigned long lastButtonTime = 0; // moment of the last button press

// For jerk computation (variation between consecutive readings)
// -1 means "not initialized" — avoids a false jerk on the first reading
float         prevMagRaw = -1;
unsigned long prevTime   = 0;

// ═════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═════════════════════════════════════════════════════════════

// Triggers the buzzer N times
// Inverted logic: LOW turns the buzzer on (controls the GND connection)
// Patterns: 1=startup | 3=medium confidence | 5=high | 10=emergency
void triggerBuzzer(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, LOW);  // on
    delay(500);
    digitalWrite(BUZZER_PIN, HIGH); // off
    delay(300);
  }
}

// Configures all the MPU6050 registers
// Called in setup() and automatically when the sensor stops responding
void initMPU() {
  // 1. Wake the sensor — factory default is sleep mode
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0x00);  // 0x00 = disable sleep
  Wire.endTransmission(true);
  delay(100); // wait to stabilize

  // 2. Configure the internal digital low-pass filter (DLPF)
  // 0x02 = 94Hz bandwidth, 3ms delay
  // 94Hz preserves the impact peak (a 50–100ms event)
  // More restrictive filters (e.g. 10Hz) would attenuate the fall signal
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A);  // CONFIG register
  Wire.write(0x02);  // DLPF_CFG = 2 → 94Hz
  Wire.endTransmission(true);

  // 3. Accelerometer scale: ±4g
  // ±4g chosen because real falls produce 2.5g–3.8g
  // ±2g would saturate at 32,767, distorting the impact reading
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);  // ACCEL_CONFIG register
  Wire.write(0x08);  // AFS_SEL = 1 → ±4g (sensitivity: 8,192 LSB/g)
  Wire.endTransmission(true);

  // 4. Gyroscope scale: ±500°/s
  // Resulting sensitivity: 65.5 LSB per degree/second
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);  // GYRO_CONFIG register
  Wire.write(0x08);  // FS_SEL = 1 → ±500°/s
  Wire.endTransmission(true);

  Serial.println("MPU6050 initialized.");
}

// Reads 14 bytes from the sensor in a single I2C transaction:
// Bytes 1-6:  acceleration (ax, ay, az — 2 bytes each, big-endian)
// Bytes 7-8:  internal temperature (discarded — must be read)
// Bytes 9-14: gyroscope (gx, gy, gz — 2 bytes each, big-endian)
// Returns false if fewer than 14 bytes were received (I2C failure)
bool readSensor(int16_t &ax, int16_t &ay, int16_t &az,
                int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);            // ACCEL_XOUT_H register — start of the data
  Wire.endTransmission(false); // false = no STOP, keep the bus held
  uint8_t n = Wire.requestFrom(MPU_ADDR, 14, true);
  if (n < 14) return false;    // failure: sensor did not respond correctly

  // Rebuild each 16-bit value from 2 bytes (big-endian):
  // Wire.read() << 8 = shifts the most significant byte (MSB) 8 positions
  // | Wire.read()    = combines with the least significant byte (LSB)
  ax = Wire.read() << 8 | Wire.read(); // X axis (lateral)
  ay = Wire.read() << 8 | Wire.read(); // Y axis (front/back)
  az = Wire.read() << 8 | Wire.read(); // Z axis (vertical/gravity)

  // Temperature (2 bytes): must be read — otherwise the gyroscope is offset
  Wire.read(); Wire.read();

  gx = Wire.read() << 8 | Wire.read(); // gyroscope X axis
  gy = Wire.read() << 8 | Wire.read(); // gyroscope Y axis
  gz = Wire.read() << 8 | Wire.read(); // gyroscope Z axis
  return true;
}

// Resets all algorithm flags
// Called after: confirmed fall, false positive, or timeout
void resetState() {
  freeFallDetected       = false;
  rotationDetectedInFall = false;
  impactDetected         = false;
}

// Connects to WiFi — blocking in setup, acceptable since it runs only once
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" Connected! IP: ");
  Serial.println(WiFi.localIP());
}

// Connects to the MQTT broker — retries until successful
void connectMQTT() {
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect("ESP32_Bracelet", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println(" Connected!");
      // Publish online status so Home Assistant knows the device is active
      mqtt.publish("bracelet/status", "online");
    } else {
      Serial.print(" Failed, rc="); Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

// ═════════════════════════════════════════════════════════════
// SETUP — runs ONCE when the ESP32 powers on
// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // I2C on the ESP32 default pins: SDA=GPIO21, SCL=GPIO22
  Wire.begin(21, 22);

  // Button: internal pull-up — HIGH at rest, LOW when pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Buzzer: starts OFF (inverted logic: HIGH = off)
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  initMPU();
  connectWiFi();
  connectMQTT();

  prevTime = millis();

  // 1 startup beep — confirms the buzzer is working
  triggerBuzzer(1);

  Serial.println("Starting up... waiting for warmup.");
}

// ═════════════════════════════════════════════════════════════
// LOOP — runs CONTINUOUSLY (~20 times per second)
// ═════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();
  int16_t ax, ay, az, gx, gy, gz;

  // Keep the MQTT connection alive (auto-reconnects if it drops)
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop(); // processes incoming MQTT messages

  // ── MANUAL EMERGENCY BUTTON ───────────────────────────────
  // 3s software debounce — avoids multiple triggers from vibration
  if (digitalRead(BUTTON_PIN) == LOW &&
      now - lastButtonTime > 3000) {
    Serial.println(">>> EMERGENCY BUTTON PRESSED! <<<");
    mqtt.publish("bracelet/emergency", "manual_button");
    triggerBuzzer(10); // 10 beeps = emergency pattern
    lastButtonTime = now;
  }

  // ── POST-FALL LOCKOUT ─────────────────────────────────────
  // After an alert, ignore new detections for POST_FALL_LOCKOUT ms
  // IMPORTANT: even during the lockout, update prevMagRaw
  // to avoid a false jerk when the lockout ends
  if (now - lastFallTime < POST_FALL_LOCKOUT) {
    if (readSensor(ax, ay, az, gx, gy, gz)) {
      prevMagRaw     = sqrt((float)ax*ax + (float)ay*ay + (float)az*az);
      prevTime       = now;
      i2cFailureCount = 0;
    }
    delay(50);
    return;
  }

  // ── READ WITH AUTO-RECOVERY ──────────────────────────────
  // The MPU6050 may enter sleep after current spikes (impacts)
  // After MAX_FAILURES consecutive failures, reinitialize automatically
  if (!readSensor(ax, ay, az, gx, gy, gz)) {
    i2cFailureCount++;
    prevTime = now; // avoid accumulated dt on the next reading
    Serial.print("!! I2C failure #"); Serial.println(i2cFailureCount);
    if (i2cFailureCount >= MAX_FAILURES) {
      Serial.println("!! Sensor went to sleep — reinitializing MPU...");
      initMPU();
      i2cFailureCount = 0;
      warmupCount     = 0; // redo warmup after reinitializing
      prevMagRaw      = -1;
      resetState();
    }
    delay(50);
    return;
  }
  i2cFailureCount = 0; // read ok — reset counter

  // ── CALCULATIONS ──────────────────────────────────────────

  // Magnitude = √(ax² + ay² + az²)
  // Represents the total acceleration intensity regardless of orientation
  float magRaw  = sqrt((float)ax*ax + (float)ay*ay + (float)az*az);

  // Gyroscope magnitude = total angular velocity
  float magGyro = sqrt((float)gx*gx + (float)gy*gy + (float)gz*gz);

  // Jerk = change in magnitude per second
  // max(..., 0.001f) guarantees dt > 0, avoiding division by zero
  float dt   = max((float)(now - prevTime) / 1000.0f, 0.001f);
  float jerk = (prevMagRaw >= 0) ? abs(magRaw - prevMagRaw) / dt : 0;
  prevMagRaw = magRaw;
  prevTime   = now;

  // ── WARMUP ────────────────────────────────────────────────
  // Ignore the first WARMUP_READINGS readings
  // Without this, the first jerk may be high and trigger a false fall
  if (warmupCount < WARMUP_READINGS) {
    warmupCount++;
    if (warmupCount == WARMUP_READINGS)
      Serial.println("System ready for monitoring.");
    delay(50);
    return;
  }

  // Debug — visible on the Serial Monitor (115200 baud)
  Serial.print("Accel:"); Serial.print(magRaw, 0);
  Serial.print(" Gyro:"); Serial.print(magGyro, 0);
  Serial.print(" Jerk:"); Serial.print(jerk, 0);
  Serial.print(" FF:"); Serial.print(freeFallDetected);
  Serial.print(" ROT:"); Serial.print(rotationDetectedInFall);
  Serial.print(" IMP:"); Serial.println(impactDetected);

  // ══════════════════════════════════════════════════════════
  // FALL DETECTION ALGORITHM — 3 PHASES
  // ══════════════════════════════════════════════════════════

  // ── PHASE 1: FREE FALL ────────────────────────────────────
  // Low magnitude + high jerk = sharp transition into a fall
  // The high jerk ensures it is not simply a tilted sensor
  if (!freeFallDetected &&
      magRaw < FREE_FALL_THRESHOLD &&
      jerk   > JERK_THRESHOLD) {
    freeFallDetected       = true;
    rotationDetectedInFall = false;
    impactDetected         = false;
    freeFallTime           = now;
    Serial.println("-- Phase 1: free fall detected");
  }

  // ── DURING FREE FALL ──────────────────────────────────────
  if (freeFallDetected && !impactDetected) {

    // High gyroscope during free fall = body rotation
    // Increases confidence but is not mandatory
    // (slow elderly falls may have little rotation)
    if (magGyro > GYRO_THRESHOLD) rotationDetectedInFall = true;

    // ── PHASE 2: IMPACT ────────────────────────────────────
    // High magnitude within the time window after free fall
    if (magRaw > IMPACT_THRESHOLD &&
        now - freeFallTime < TIME_WINDOW) {
      impactDetected = true;
      impactTime     = now;
      Serial.println("-- Phase 2: impact detected");
    }

    // Timeout: free fall without impact in the window = false positive
    if (now - freeFallTime > TIME_WINDOW) {
      Serial.println("-- Timeout: no impact, cancelling");
      resetState();
    }
  }

  // ── PHASE 3: CONFIRMATION BY REST ────────────────────────
  // After the impact, wait for the body to stay still for REST_DURATION
  // Sharp gestures: body keeps moving → discarded
  // Real falls: body still on the floor → confirmed
  if (impactDetected &&
      now - impactTime > REST_DURATION) {

    bool bodyStill = (magRaw < REST_THRESHOLD);

    if (bodyStill) {
      // FALL CONFIRMED
      if (rotationDetectedInFall) {
        // High confidence: all 3 phases + rotation detected
        Serial.println(">>> FALL CONFIRMED (high confidence) <<<");
        mqtt.publish("bracelet/fall", "high_confidence");
        triggerBuzzer(5); // 5 beeps
      } else {
        // Medium confidence: 3 phases without rotation — still confirmed
        // May be a slow elderly fall with little body rotation
        Serial.println(">>> FALL CONFIRMED (medium confidence) <<<");
        mqtt.publish("bracelet/fall", "medium_confidence");
        triggerBuzzer(3); // 3 beeps
      }
      lastFallTime = now; // start the post-fall lockout
    } else {
      // Body still moving after the rest time = false positive
      Serial.println("-- Discarded: body did not settle after impact");
    }

    // In both cases, reset for the next monitoring cycle
    resetState();
  }

  // Each cycle lasts ~50ms = 20 readings per second
  delay(50);
}
