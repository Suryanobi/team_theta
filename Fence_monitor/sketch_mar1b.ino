/*
 ==========================================
  FENCE LINE MONITOR
  WITH LCD + POWER THEFT DETECTION
 ==========================================
  V1 (Before Voltage) -> A0
  V2 (After Voltage)  -> A1
  C1 (Before Current) -> A2
  C2 (After Current)  -> A3
  LCD I2C -> SDA=A4, SCL=A5
 ==========================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Change 0x27 to 0x3F if LCD not working
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- PIN DEFINITIONS ----
#define V1_PIN A0
#define V2_PIN A1
#define C1_PIN A2
#define C2_PIN A3

// ---- SETTINGS ----
#define VOLTAGE_SCALE     5.0        // 0-25V module
#define ACS_SENSITIVITY   0.185      // 5A module

#define VOLTAGE_THRESHOLD 1.0
#define CURRENT_THRESHOLD 0.02
#define THEFT_THRESHOLD   0.02

#define SAMPLE_COUNT 20
#define READ_INTERVAL 1000

// ---- AUTO CALIBRATION VALUES ----
float ACS_OFFSET1 = 0;
float ACS_OFFSET2 = 0;

// =============================
// READ VOLTAGE
// =============================
float readVoltage(int pin) {
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(pin);
  }
  float avg = sum / (float)SAMPLE_COUNT;
  float adcVoltage = (avg / 1023.0) * 5.0;
  return adcVoltage * VOLTAGE_SCALE;
}

// =============================
// READ CURRENT (CALIBRATED)
// =============================
float readCurrent(int pin, float offset) {
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(pin);
  }
  float avg = sum / (float)SAMPLE_COUNT;
  float voltage = (avg - offset) * (5.0 / 1023.0);
  float current = voltage / ACS_SENSITIVITY;
  return abs(current);
}

// =============================
void calibrateACS() {
  Serial.println("Calibrating Current Sensors...");
  delay(2000);

  for (int i = 0; i < 200; i++) {
    ACS_OFFSET1 += analogRead(C1_PIN);
    ACS_OFFSET2 += analogRead(C2_PIN);
    delay(5);
  }

  ACS_OFFSET1 /= 200.0;
  ACS_OFFSET2 /= 200.0;

  Serial.println("Calibration Complete.");
}

// =============================
void setup() {

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Calibrating...");
  calibrateACS();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Fence Monitor");
  delay(2000);
  lcd.clear();

  Serial.println("V1   V2   C1   C2   STATUS");
  Serial.println("--------------------------------");
}

// =============================
void loop() {

  float V1 = readVoltage(V1_PIN);
  float V2 = readVoltage(V2_PIN);
  float C1 = readCurrent(C1_PIN, ACS_OFFSET1);
  float C2 = readCurrent(C2_PIN, ACS_OFFSET2);

  String status = "UNKNOWN";

  // ---- LOGIC ----
  if (V1 < VOLTAGE_THRESHOLD) {
    status = "SUPPLY_FAIL";
  }

  else if (V1 > VOLTAGE_THRESHOLD &&
           V2 < VOLTAGE_THRESHOLD &&
           C2 < CURRENT_THRESHOLD) {
    status = "FENCE_CUT";
  }

  else if (V1 > VOLTAGE_THRESHOLD &&
           V2 > VOLTAGE_THRESHOLD &&
           C1 < CURRENT_THRESHOLD &&
           C2 < CURRENT_THRESHOLD) {
    status = "LINE_OPEN";
  }

  else if (V1 > VOLTAGE_THRESHOLD &&
           V2 > VOLTAGE_THRESHOLD &&
           C1 > CURRENT_THRESHOLD &&
           (C1 - C2) > THEFT_THRESHOLD) {
    status = "THEFT";
  }

  else if (V1 > VOLTAGE_THRESHOLD &&
           V2 > VOLTAGE_THRESHOLD &&
           C1 > CURRENT_THRESHOLD &&
           C2 > CURRENT_THRESHOLD) {
    status = "ALL_OK";
  }

  // ---- SERIAL OUTPUT ----
  Serial.print(V1,2); Serial.print("   ");
  Serial.print(V2,2); Serial.print("   ");
  Serial.print(C1,3); Serial.print("   ");
  Serial.print(C2,3); Serial.print("   ");
  Serial.println(status);

  // ---- LCD DISPLAY ----
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print(status);

  lcd.setCursor(0,1);
  lcd.print("V:");
  lcd.print(V1,1);
  lcd.print(" I:");
  lcd.print(C1,2);

  delay(READ_INTERVAL);
}