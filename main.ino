// ============================================================
//  ARK - Automated Wastepaper Recycling Machine
//  STI College Santa Rosa | BS Computer Engineering
//
//  External Libraries required:
//    - LiquidCrystal I2C by Frank de Brabander
//    - HX711 Arduino Library by Bogdan Necula
//
//  RECENT UPDATES:
//    - Added HC-SR04 ultrasonic sensor for true presence detection
//    - Fixed dryingState() to use a timeout loop instead of a
//      one-shot delay (previously could loop forever if PIR
//      triggered repeatedly)
//    - UV stays off if PIR detects motion OR ultrasonic detects
//      something within SAFE_DISTANCE_CM
//    - Drying timer pauses while person is detected, resumes
//      when area is clear
//    - RESET button now works during drying stage
//    - Added Serial debug output for easier testing without LCD
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HX711.h"

// ── LCD (I2C) ───────────────────────────────────────────────
// If display stays blank, change 0x27 to 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── Load Cell ───────────────────────────────────────────────
HX711 scale;
#define HX_DT   3
#define HX_SCK  2

// ── Buttons ─────────────────────────────────────────────────
#define BTN_START  4   // Pin 4 to GND
#define BTN_RESET  5   // Pin 5 to GND

// ── PIR Sensor ──────────────────────────────────────────────
#define PIR_PIN    6

// ── HC-SR04 Ultrasonic Sensor ───────────────────────────────
#define TRIG_PIN   30
#define ECHO_PIN   31

// ── UV Relay ────────────────────────────────────────────────
#define UV_RELAY   7

// ── Shredder Motor (L298N Channel A) ────────────────────────
#define SHRED_IN1  8
#define SHRED_IN2  9
#define SHRED_EN   10   // PWM

// ── Mixer Motor (L298N Channel B) ───────────────────────────
#define MIX_IN1    11
#define MIX_IN2    12
#define MIX_EN     13   // PWM

// ── Press Motor (second L298N or same board Channel A) ──────
#define PRESS_IN1  22
#define PRESS_IN2  23
#define PRESS_EN   44   // PWM (Pin 44 is PWM on Mega)

// ── Tunable Parameters ──────────────────────────────────────
float CAL_FACTOR       = 2280.0;  // Calibrate with known weight before demo
float MIN_WEIGHT       = 50.0;    // Minimum grams of paper to proceed (g)
int   SAFE_DISTANCE_CM = 50;      // Ultrasonic threshold — UV off if < 50 cm
unsigned long DRY_DURATION_MS = 20000UL; // Total UV drying time (ms)

// ── State Machine ───────────────────────────────────────────
enum State {
  WEIGHING,
  READY,
  SHREDDING,
  PULPING,
  PRESSING,
  DRYING,
  COMPLETE
};

State machineState = WEIGHING;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println("ARK System Starting...");

  // Buttons (active LOW with internal pull-up)
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Sensors
  pinMode(PIR_PIN,  INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Relay
  pinMode(UV_RELAY, OUTPUT);
  digitalWrite(UV_RELAY, LOW);  // UV OFF on boot

  // Shredder motor
  pinMode(SHRED_IN1, OUTPUT);
  pinMode(SHRED_IN2, OUTPUT);
  pinMode(SHRED_EN,  OUTPUT);

  // Mixer motor
  pinMode(MIX_IN1, OUTPUT);
  pinMode(MIX_IN2, OUTPUT);
  pinMode(MIX_EN,  OUTPUT);

  // Press motor
  pinMode(PRESS_IN1, OUTPUT);
  pinMode(PRESS_IN2, OUTPUT);
  pinMode(PRESS_EN,  OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  ARK System  ");
  lcd.setCursor(0, 1);
  lcd.print(" Initializing ");
  delay(2000);
  lcd.clear();

  // Load cell
  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(CAL_FACTOR);
  scale.tare();

  Serial.println("Setup complete. Entering WEIGHING state.");
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  // RESET button is checked globally at all times
  if (!digitalRead(BTN_RESET)) {
    resetSystem();
    return;
  }

  switch (machineState) {
    case WEIGHING:  weighingState();  break;
    case READY:     readyState();     break;
    case SHREDDING: shreddingState(); break;
    case PULPING:   pulpingState();   break;
    case PRESSING:  pressingState();  break;
    case DRYING:    dryingState();    break;
    case COMPLETE:  completeState();  break;
  }
}

// ============================================================
//  ULTRASONIC DISTANCE HELPER
//  Returns distance in centimeters.
//  Returns 999 if no echo received (out of range / clear).
// ============================================================
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 999; // No echo = nothing detected
  return duration * 0.034 / 2;  // Convert to cm
}

// ============================================================
//  PERSON DETECTION
//  Returns true if PIR sees motion OR ultrasonic sees presence
// ============================================================
bool personDetected() {
  bool pirTriggered  = digitalRead(PIR_PIN);
  long distance      = getDistance();
  bool tooClose      = (distance < SAFE_DISTANCE_CM);

  if (pirTriggered || tooClose) {
    Serial.print("Person detected! PIR=");
    Serial.print(pirTriggered);
    Serial.print(" Distance=");
    Serial.print(distance);
    Serial.println("cm");
    return true;
  }
  return false;
}

// ============================================================
//  STATE: WEIGHING
//  Continuously reads weight. Advances to READY when >= MIN_WEIGHT.
// ============================================================
void weighingState() {
  float weight = scale.get_units(5);
  if (weight < 0) weight = 0; // Clamp negative drift

  lcd.setCursor(0, 0);
  lcd.print("Weight: ");
  lcd.print(weight, 1);
  lcd.print("g   ");

  lcd.setCursor(0, 1);
  if (weight >= MIN_WEIGHT) {
    lcd.print("Ready!        ");
    Serial.println("Sufficient weight. Moving to READY.");
    delay(500);
    machineState = READY;
  } else {
    lcd.print("Add Paper...  ");
  }

  delay(200);
}

// ============================================================
//  STATE: READY
//  Waits for user to press START button.
// ============================================================
void readyState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight OK!");
  lcd.setCursor(0, 1);
  lcd.print("Press START   ");

  Serial.println("Waiting for START button...");

  while (machineState == READY) {
    if (!digitalRead(BTN_RESET)) { resetSystem(); return; }
    if (!digitalRead(BTN_START)) {
      Serial.println("START pressed. Moving to SHREDDING.");
      delay(50); // debounce
      machineState = SHREDDING;
    }
  }
}

// ============================================================
//  STATE: SHREDDING
//  Runs shredder motor for a fixed duration then advances.
// ============================================================
void shreddingState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Shredding...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait   ");

  Serial.println("Shredding started.");
  motorForward(SHRED_IN1, SHRED_IN2, SHRED_EN, 200);
  delay(10000); // Placeholder — adjust after testing
  motorStop(SHRED_IN1, SHRED_IN2, SHRED_EN);

  Serial.println("Shredding complete. Moving to PULPING.");
  machineState = PULPING;
}

// ============================================================
//  STATE: PULPING
//  Runs mixer motor for a fixed duration then advances.
// ============================================================
void pulpingState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pulping...");
  lcd.setCursor(0, 1);
  lcd.print("Mixing fibers ");

  Serial.println("Pulping started.");
  motorForward(MIX_IN1, MIX_IN2, MIX_EN, 180);
  delay(15000); // Placeholder — adjust after testing
  motorStop(MIX_IN1, MIX_IN2, MIX_EN);

  Serial.println("Pulping complete. Moving to PRESSING.");
  machineState = PRESSING;
}

// ============================================================
//  STATE: PRESSING
//  Runs press motor for a fixed duration then advances.
// ============================================================
void pressingState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pressing...");
  lcd.setCursor(0, 1);
  lcd.print("Forming sheet ");

  Serial.println("Pressing started.");
  motorForward(PRESS_IN1, PRESS_IN2, PRESS_EN, 180);
  delay(10000); // Placeholder — adjust after testing
  motorStop(PRESS_IN1, PRESS_IN2, PRESS_EN);

  Serial.println("Pressing complete. Moving to DRYING.");
  machineState = DRYING;
}

// ============================================================
//  STATE: DRYING
//  UV light runs for DRY_DURATION_MS total.
//  Timer PAUSES when a person is detected (PIR or ultrasonic).
//  Timer RESUMES when area is clear.
//  RESET button still works during drying.
// ============================================================
void dryingState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Drying...");

  Serial.println("Drying started.");

  unsigned long elapsed   = 0;
  unsigned long lastTick  = millis();

  while (elapsed < DRY_DURATION_MS) {

    // Always allow RESET during drying
    if (!digitalRead(BTN_RESET)) {
      digitalWrite(UV_RELAY, LOW);
      resetSystem();
      return;
    }

    if (personDetected()) {
      // Person nearby — UV off, pause timer
      digitalWrite(UV_RELAY, LOW);
      lcd.setCursor(0, 1);
      lcd.print("Safety STOP!  ");
      lastTick = millis(); // Reset tick so elapsed doesn't increment
      delay(500);
    } else {
      // Area clear — UV on, advance timer
      digitalWrite(UV_RELAY, HIGH);
      lcd.setCursor(0, 1);

      // Show remaining time in seconds on LCD
      unsigned long remaining = (DRY_DURATION_MS - elapsed) / 1000;
      lcd.print("UV ON  ");
      lcd.print(remaining);
      lcd.print("s rem   ");

      unsigned long now = millis();
      elapsed += (now - lastTick);
      lastTick = now;

      Serial.print("Drying elapsed: ");
      Serial.print(elapsed / 1000);
      Serial.println("s");

      delay(500);
    }
  }

  digitalWrite(UV_RELAY, LOW);
  Serial.println("Drying complete. Moving to COMPLETE.");
  machineState = COMPLETE;
}

// ============================================================
//  STATE: COMPLETE
//  Signals the user that the cycle is done.
// ============================================================
void completeState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  COMPLETE!   ");
  lcd.setCursor(0, 1);
  lcd.print("Press RESET   ");

  Serial.println("Cycle complete. Waiting for RESET.");
  delay(500);
}

// ============================================================
//  MOTOR HELPERS
// ============================================================
void motorForward(int in1, int in2, int en, int speedVal) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(en, speedVal);
}

void motorStop(int in1, int in2, int en) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(en, 0);
}

// ============================================================
//  RESET SYSTEM
//  Safe shutdown: UV off, motors off, re-tare scale, back to WEIGHING.
// ============================================================
void resetSystem() {
  Serial.println("RESET triggered.");

  // UV off
  digitalWrite(UV_RELAY, LOW);

  // All motors off
  motorStop(SHRED_IN1, SHRED_IN2, SHRED_EN);
  motorStop(MIX_IN1,   MIX_IN2,   MIX_EN);
  motorStop(PRESS_IN1, PRESS_IN2, PRESS_EN);

  // Re-tare scale
  scale.tare();

  // Back to start
  machineState = WEIGHING;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  ARK System  ");
  lcd.setCursor(0, 1);
  lcd.print("   Reset OK   ");
  delay(1500);
  lcd.clear();
}
