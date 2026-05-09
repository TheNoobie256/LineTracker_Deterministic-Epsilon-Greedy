#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <TRSensors.h>

// --- Hardware Pins ---
#define RGB_PIN     7
#define TRIG        3
#define ECHO        2

#define PWMA 6    
#define AIN2 A0   
#define AIN1 A1   
#define PWMB 5    
#define BIN1 A2   
#define BIN2 A3   

Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, RGB_PIN, NEO_GRB + NEO_KHZ800);
TRSensors trs = TRSensors();
unsigned int sensorValues[5];

// ==========================================
// PID TUNING VARIABLES
// ==========================================
float Kp = 0.08; 
float Kd = 0.8;
int lastError = 0;

int baseSpeed = 50;       // Constant safe turning speed
int maxSpeed = 90;        // Absolute limit for PID spikes

// --- STRATEGY VARIABLES ---
unsigned long lastDecisionTime = 0;
int currentBiasError = -3000; // Always default to Left
int activeOverride = 0;       // Locks in the turn decision

// --- THE LOOP BREAKER (Counter + Timer) ---
int consecutiveLefts = 0;
int maxConsecutiveLefts = 4;  // Break out after 4 lefts

unsigned long loopStartTime = 0;
unsigned long maxLoopDuration = 5000; 

// --- THE CURIOSITY ENGINE ---
// 20% chance to make a completely random decision.
int curiosityChance = 20; 

void setup() {
  Serial.begin(115200);
  
  strip.begin();
  setLEDs(255, 100, 0); // Orange - Calibrating
  
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // Seed the random number generator using analog noise
  randomSeed(analogRead(A4));

  calibrateRobot();
}

void loop() {
  // 1. OBSTACLE DETECTION (8 cm)
  long distance = getDistance();
  if (distance > 0 && distance < 8) {
    setLEDs(255, 0, 0); // Red
    
    // Slam the brakes
    driveMotors(0, 0); 
    delay(50);
    
    driveMotors(-60, -60); 
    delay(200); 
    
    driveMotors(70, -70); 
    delay(400); // Blind spin to clear the line
    
    unsigned long spinTimer = millis();
    while (millis() - spinTimer < 2000) { 
      trs.readLine(sensorValues);
      if (sensorValues[2] > 400) break; 
    }
    
    // Reset the tracking logic because we hit a dead end!
    consecutiveLefts = 0; 
    return; 
  } 

  // 2. Read Sensors
  unsigned int position = trs.readLine(sensorValues);
  
  // 3. Calculate Base Error
  int error = position - 2000;

  // 4. THE MASTER INTERSECTION HANDLING
  bool leftSeesLine = sensorValues[0] > 600;
  bool rightSeesLine = sensorValues[4] > 600;

  // CHANGED: Now uses || (OR) so it triggers on ANY branch or corner!
  if (leftSeesLine || rightSeesLine) {

    unsigned long currentTime = millis();
    
    // 500ms Debounce to prevent double-counting thick tape
    if (currentTime - lastDecisionTime > 500) {
      lastDecisionTime = currentTime;
      
      // Roll the dice! (0 to 99)
      int diceRoll = random(0, 100);
      bool isCurious = false;

      // ----------------------------------------
      // STEP A: Decide what we WANT to do
      // ----------------------------------------
      if (diceRoll < curiosityChance) {
        // CURIOSITY MODE
        isCurious = true;
        int randomDirection = random(0, 2); 
        if (randomDirection == 0) currentBiasError = -3000; // Want Left
        else currentBiasError = 3000;  // Want Right
        consecutiveLefts = 0;   
      } else {
        // DISCIPLINED MODE
        consecutiveLefts++; 
        if (consecutiveLefts == 1) loopStartTime = currentTime; 

        if (consecutiveLefts >= maxConsecutiveLefts) {
          unsigned long loopDuration = currentTime - loopStartTime;
          if (loopDuration < maxLoopDuration) {
            currentBiasError = 3000; // Want Right! (Escape)
            consecutiveLefts = 0;    
          } else {
            currentBiasError = -3000; // Want Left (Normal)
            consecutiveLefts = 1;     
            loopStartTime = currentTime;
          }
        } else {
          currentBiasError = -3000; // Want Left (Normal)
        }
      }

      // ----------------------------------------
      // STEP B: The Reality Check!
      // ----------------------------------------
      if (currentBiasError == -3000 && leftSeesLine) {
        activeOverride = -3000; // Path exists, force Left!
        if (isCurious) setLEDs(255, 255, 255); else setLEDs(0, 0, 255);
      } 
      else if (currentBiasError == 3000 && rightSeesLine) {
        activeOverride = 3000;  // Path exists, force Right!
        if (isCurious) setLEDs(255, 255, 255); else setLEDs(255, 255, 0);
      } 
      else {
        // Cancel the override and let the PID naturally follow the line straight.
        activeOverride = 0; 
        setLEDs(0, 255, 0); // Green
      }
    }

    // Apply the locked override while crossing the tape
    if (activeOverride != 0) {
      error = activeOverride;
    }
    
  } else {
    // We are back on a normal single line. Clear overrides.
    setLEDs(0, 255, 0); // Green
    activeOverride = 0; 
  }

  // 5. The PID Equation
  int motorSpeedChange = (Kp * error) + (Kd * (error - lastError));
  lastError = error;

  // 6. Apply the change to the base speed
  int leftMotorSpeed = baseSpeed + motorSpeedChange;
  int rightMotorSpeed = baseSpeed - motorSpeedChange;

  // 7. CONSTRAIN WITH REVERSE CAPABILITY
  leftMotorSpeed = constrain(leftMotorSpeed, -maxSpeed, maxSpeed);
  rightMotorSpeed = constrain(rightMotorSpeed, -maxSpeed, maxSpeed);

  // 8. Drive the Motors!
  driveMotors(leftMotorSpeed, rightMotorSpeed);
}

// ==========================================
// HELPER FUNCTIONS
// ==========================================

void calibrateRobot() {
  for (int i = 0; i < 100; i++) {
    trs.calibrate();
    delay(40);
  }
  setLEDs(0, 255, 0); 
  delay(1000);
}

void driveMotors(int leftSpeed, int rightSpeed) {
  if (leftSpeed >= 0) {
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); 
    analogWrite(PWMA, leftSpeed);
  } else {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); 
    analogWrite(PWMA, -leftSpeed); 
  }
  
  if (rightSpeed >= 0) {
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, rightSpeed);
  } else {
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    analogWrite(PWMB, -rightSpeed); 
  }
}

long getDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 10000); 
  if (duration == 0) return 999;
  return duration / 58;
}

void setLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<4; i++) strip.setPixelColor(i, r, g, b);
  strip.show();
}
