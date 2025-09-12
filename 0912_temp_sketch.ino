#include <Servo.h>
Servo servo;

const int FLEX_PIN = A0;
const int RAW_LO   = 940;  // counterclockwise end
const int RAW_MID  = 960;  // center
const int RAW_HI   = 980;  // clockwise end

int mapRange(int x, int in_min, int in_max, int out_min, int out_max) {
  // linear interpolation with bounds
  if (in_max == in_min) return out_min;
  long val = (long)(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  if (val < out_min) val = out_min;
  if (val > out_max) val = out_max;
  return (int)val;
}

void setup() {
  Serial.begin(9600);
  servo.attach(9);
}

void loop() {
  int flex = analogRead(FLEX_PIN);

  int angle;
  if (flex <= RAW_LO) {
    angle = 0;
  } else if (flex >= RAW_HI) {
    angle = 180;
  } else if (flex < RAW_MID) {
    // 940..960  ->  0..90
    angle = mapRange(flex, RAW_LO, RAW_MID, 0, 90);
  } else {
    // 960..980  ->  90..180
    angle = mapRange(flex, RAW_MID, RAW_HI, 90, 180);
  }

  // Optional: small deadband to lock exactly at 90 near center
  if (abs(flex - RAW_MID) <= 2) angle = 90;

  servo.write(angle);

  Serial.print("flex=");  Serial.print(flex);
  Serial.print("  angle="); Serial.println(angle);

  delay(20);
}
