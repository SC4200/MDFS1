#include <Arduino.h>
#include <ArmSteppers.h>
#include <Claw.h>
#include <Wire.h>

// 1. pin assignments
const byte kEnPin = 8;
const byte kStepBasePin = 2;
const byte kDirBasePin = 5;
const byte kStepMiddlePin = 3;
const byte kDirMiddlePin = 6;

// servo inputs for the claw. needed here
const byte kClawPin = 7;  // change this

Claw claw(kClawPin);

byte allStepperPins[2][2] = {{kStepBasePin, kStepMiddlePin},
                             {kDirBasePin, kDirMiddlePin}};

// creating arm stepper object
ArmSteppers arm(allStepperPins);

// pod positions = {pod_x, pod_y}
const int kPodNut = 6;
const int kTree300[] = {200, 300};
const int kTree150[] = {150, 150};
const int kGround300[] = {300, kPodNut};
const int kGround200[] = {200, kPodNut};  // two of these.
const int kGround150[] = {150, kPodNut};

// 2. i2c wire
const byte kWireAddress = 9;
const byte kWireArrayLength = 3;

// 3. timing
unsigned long us_current_time;
unsigned long us_state_timer;
unsigned long us_time_since_pod_collection = 0;

// 4. joy and states inputs
int i_stepper_direction = 0;  // renaming for it's purpose
int i_joy_y = 0;
byte by_state = 0;
const int kJoyNeutral = 255 / 2;  // half of byte recieved

// 5. state machine specific code
byte state = 0;
byte last_state = 0;
byte pod_state = 0;
byte last_pod_state = 0;

// stepper
int i_step_pin = kStepMiddlePin;
int i_dir_pin = kDirMiddlePin;

// stu: should go through and comment this to be able to understand what is going on.
void readData(int num) {
  if (num >= kWireArrayLength) {
    byte data[kWireArrayLength];
    for (int i = 0; i < kWireArrayLength; i++) {
      data[i] = Wire.read();
    }
    i_joy_y = data[0];
    i_stepper_direction = data[1];
    by_state = data[2];
    Serial.println(i_stepper_direction);  // test
  }
}

void setup() {
  pinMode(kEnPin, OUTPUT);
  digitalWrite(kEnPin, LOW);
  Wire.begin(kWireAddress);
  Wire.onReceive(readData);
  Serial.begin(9600);
}

// Ensures time is only seen once per state.
void stateDuration(int timing) {
  if (state != last_state || pod_state != last_pod_state) {
    us_state_timer = us_current_time + timing;
  }
  last_state = state;
  last_pod_state = pod_state;
}

// This progresses all states after given timer.
void stateProgression() {
  if (us_state_timer < us_current_time && state != 0) {
    if (pod_state != 0) {
      pod_state++;
    } else {
      state++;
    }
  }
}

// Repeated tasks for collecting and depositing a single pod.
void collectPod(const int collect_pod[2]) {
  switch (pod_state) {
    case 0:
      // timer prevents a loop
      if (us_current_time > (us_time_since_pod_collection + 2000)) {
        pod_state++;
      }
    case 1:
      stateDuration(100);
      claw.open();
    case 2:
      stateDuration(1000);
      arm.moveTo(collect_pod);  // move towards specific pod
    case 3:
      stateDuration(500);
      claw.close();
    case 4:  // move to deposit location
      stateDuration(1000);
      arm.deposit();
    case 5:
      claw.open();  // release pod into chassis path
      us_time_since_pod_collection = us_current_time;
      pod_state = 0;
  }
}

// Main State Machine. Testing required but relies on timing to reach each
// location.
void stateMachine() { // stu: make sure every state is labeled in an easy to understand way.
  switch (state) {
    case 0:  // waits for button press to begin track.
    case 1:
      // move to platform
      stateDuration(1000);
    case 2:
      // move to first pod
      stateDuration(500);
    case 3:
      stateDuration(200);
      collectPod(kGround150);
    case 4:
      // move to 1st tree
    case 5:
      collectPod(kTree150);
    case 6:
      // move to ground pod
    case 7:
      collectPod(kGround200);
    case 8:
      // travel across to next platform
    case 9:
      collectPod(kGround300);
    case 10:
      // move to 2nd tree
    case 11:
      collectPod(kTree300);
    case 12:
      // move to final pod
    case 13:
      collectPod(kGround200);
    case 14:
      // move to deposit hole
  }
  stateProgression();
}

void loop() {
  us_current_time = millis();

  stateMachine();
  arm.moving();
}
