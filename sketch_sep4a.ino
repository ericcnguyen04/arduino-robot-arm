/*
  Flex → Servo Boilerplate
  - Reads multiple flex sensors (analog)
  - Maps to servo angles with per-channel calibration + smoothing
  - Quick-start calibration on boot (collects baseline over CALIB_MS)
  - Optional inversion per channel (open/close direction)
  
  WIRING (typical flex sensor):
    Flex sensor +5V → one leg of a voltage divider
    Flex sensor other leg → A0..A4 (signal)
    Divider resistor (e.g., 22k–47k) from signal → GND
  POWER:
    Power servos from a separate 5–6V supply (shared GND with Arduino).
*/

#include <Servo.h>

// ---- USER SETTINGS ----
const uint8_t N_CH = 5;                           // number of flex/servo channels
const uint8_t FLEX_PINS[N_CH]  = {A0, A1, A2, A3, A4}; // flex analog pins
const uint8_t SERVO_PINS[N_CH] = {3, 5, 6, 9, 10};     // servo control pins

// Servo angle limits (mechanical safety)
const int SERVO_MIN_ANGLE[N_CH] = {0, 0, 0, 0, 0};
const int SERVO_MAX_ANGLE[N_CH] = {180, 180, 180, 180, 180};

// Flex calibration (raw ADC). Will be auto-filled at startup, but you can hardcode later.
int flexMin[N_CH];   // raw value for "open" (less bent)
int flexMax[N_CH];   // raw value for "closed" (more bent)

// Optional inversion per channel (true flips direction)
const bool INVERT[N_CH] = {false, false, false, false, false};

// Smoothing (EMA)
const float ALPHA = 0.25f;     // 0=no smoothing, 1=noisy; try 0.2–0.4
float smoothed[N_CH] = {0};    // internal state

// Debounce/deadzone to ignore tiny jitters (in raw ADC units)
const int DEADZONE = 4;

// Calibration timing
const unsigned long CALIB_MS = 1500;  // time to wiggle sensors at boot

// Serial debug
const bool DEBUG = false;
const unsigned long DEBUG_MS = 200;
unsigned long lastDebug = 0;

// ---- INTERNALS ----
Servo servos[N_CH];

int mapConstrained(int x, int inMin, int inMax, int outMin, int outMax) {
  // protect from divide-by-zero and reverse ranges
  if (inMax == inMin) return outMin;
  long v = (long)(x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
  if (v < outMin) v = outMin;
  if (v > outMax) v = outMax;
  return (int)v;
}

void attachServos() {
  for (uint8_t i = 0; i < N_CH; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(90); // safe neutral
  }
}

void autoCalibrate() {
  // Initialize with current reads
  for (uint8_t i = 0; i < N_CH; i++) {
    int r = analogRead(FLEX_PINS[i]);
    flexMin[i] = r;
    flexMax[i] = r;
  }

  unsigned long t0 = millis();
  while (millis() - t0 < CALIB_MS) {
    for (uint8_t i = 0; i < N_CH; i++) {
      int r = analogRead(FLEX_PINS[i]);
      if (r < flexMin[i]) flexMin[i] = r;
      if (r > flexMax[i]) flexMax[i] = r;
    }
  }

  // Expand a little margin to avoid clipping
  for (uint8_t i = 0; i < N_CH; i++) {
    flexMin[i] = max(0, flexMin[i] - 5);
    flexMax[i] = min(1023, flexMax[i] + 5);
  }
}

void setup() {
  if (DEBUG) Serial.begin(115200);
  for (uint8_t i = 0; i < N_CH; i++) pinMode(FLEX_PINS[i], INPUT);
  attachServos();
  delay(250);
  autoCalibrate();

  // Seed smoothing with current readings
  for (uint8_t i = 0; i < N_CH; i++) {
    smoothed[i] = analogRead(FLEX_PINS[i]);
  }
}

void loop() {
  for (uint8_t i = 0; i < N_CH; i++) {
    int raw = analogRead(FLEX_PINS[i]);

    // Deadzone around current smoothed value to reduce jitter
    if (abs(raw - (int)smoothed[i]) > DEADZONE) {
      smoothed[i] = ALPHA * raw + (1.0f - ALPHA) * smoothed[i];
    }

    // Choose mapping direction (invert if needed)
    int angle;
    if (!INVERT[i]) {
      angle = mapConstrained((int)smoothed[i], flexMin[i], flexMax[i],
                             SERVO_MIN_ANGLE[i], SERVO_MAX_ANGLE[i]);
    } else {
      angle = mapConstrained((int)smoothed[i], flexMin[i], flexMax[i],
                             SERVO_MAX_ANGLE[i], SERVO_MIN_ANGLE[i]);
    }

    servos[i].write(angle);
  }

  // Optional serial dump
  if (DEBUG && millis() - lastDebug > DEBUG_MS) {
    lastDebug = millis();
    for (uint8_t i = 0; i < N_CH; i++) {
      Serial.print((int)smoothed[i]); Serial.print('\t');
    }
    Serial.println();
  }

  // Small loop delay to reduce noise; adjust as needed
  delay(5);
}
