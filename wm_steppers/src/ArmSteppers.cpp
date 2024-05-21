#include "ArmSteppers.h"

#include "Arduino.h"

// stu: This should be explained too.
ArmSteppers::ArmSteppers(byte allStepperPins[2][2]) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      pinMode(allStepperPins[i][j], OUTPUT);
      switch (i) {
        case 0:
          _pinMotion[j] = allStepperPins[i][j];
        case 1:
          _pinDirection[j] = allStepperPins[i][j];
      }
    }
  }
}

// Private: Takes input x and y values to pod, changes values to include
// dimensions of the robot chassis itself.
void ArmSteppers::adjustXYvals(const int pod_pos[2]) {
  for (int i = 0; i < 2; i++) {
    _pod_pos[i] = pod_pos[i];
  }
  // x: distance from platform
  _pod_pos[0] = _pod_pos[0] + _kNodeXToPlatformX;
  // y: arm axis height, claw height
  _pod_pos[1] = _pod_pos[0] + _kAddY;
}

// Private: Trig calculations to output the intended base axis angle.
float ArmSteppers::getBaseAxisHeading() {
  // x and y initial
  float _theta_b_1 = atan2(_pod_pos[1], _pod_pos[2]) * 180 / (3.14);
  // angle within created triangle
  float _theta_b_2 =
      acos((_kArmSideA * _kArmSideA + _dist_side_c * _dist_side_c -
            _kArmSideB * _kArmSideB) /
           (2 * _kArmSideA * _dist_side_c));
  float baseAxisHeading = (180 - (_theta_b_1 + _theta_b_2));

  return baseAxisHeading;
  // issue, needs to return angle rounded to steps (whole ints)
}

// Private: Trig calculations to output the intended middle axis angle.
// stu: perhaps this could be explained better? what is the "middle axis" 
float ArmSteppers::getMiddleAxisHeading() {
  float _theta_c = acos((_kArmSideA * _kArmSideA + _kArmSideB * _kArmSideB -
                         _dist_side_c * _dist_side_c) /
                        (2 * _kArmSideA * _kArmSideB));

  float middleAxisHeading = (180 - (_theta_c));

  return middleAxisHeading;
  // issue, needs to return angle rounded to steps (whole ints)
}

// Private: Finds distance arm must travel from the given x and y inputs
float ArmSteppers::getSideCLength() {
  float sideCLength = sqrt(_pod_pos[kBaseAxis] * _pod_pos[kBaseAxis] +
                           _pod_pos[kMiddleAxis] * _pod_pos[kMiddleAxis]);
  return sideCLength;
}

// Public: User will use this to move arm to each pod location.
void ArmSteppers::moveTo(const int pod_pos[2]) {
  adjustXYvals(pod_pos);
  _dist_side_c = getSideCLength();
  _armheading[kBaseAxis] = getBaseAxisHeading();
  _armheading[kMiddleAxis] = getMiddleAxisHeading();
}

// Public: adjusted moveTo function, will require testing and value change.
void ArmSteppers::deposit() {
  const int _deposit_x_y[2] = {0, 50};
  moveTo(_deposit_x_y);
}

// Private: Has arm axis reached its destination?
bool ArmSteppers::isStepperEndpointMet() {
  if (_armheading[b_which_stepper] == _armcurrent[b_which_stepper]) {
    return true;
  }
  return false;
}

// Private: Checks which direction to move
bool ArmSteppers::getDirection() {
  if (_armheading[b_which_stepper] > _armcurrent[b_which_stepper]) {
    return true;
  }
  return false;
}

// Private: Writes direction for both steppers
void ArmSteppers::adjustDirection() {
  digitalWrite(_pinDirection[b_which_stepper], getDirection());
}

// Private: after moving a step, this records the change.
void ArmSteppers::changeCurrentPosition(bool has_step_occured) {
  if (has_step_occured == true) {
    if (getDirection() == true) {
      _armcurrent[b_which_stepper]--;
      // test this with a serial print.
    } else {
      _armcurrent[b_which_stepper]++;
    }
  }
}

// Private: Alternates which stepper is moving after period T
void ArmSteppers::nextStepPowered(bool b_is_power_on) {
  if (us_current_time - us_last_stepper_motion >= f_T) {
    bool b_alternating_steppers = b_which_stepper;

    digitalWrite(_pinMotion[b_alternating_steppers], b_is_power_on);
    digitalWrite(_pinMotion[!b_alternating_steppers], LOW);

    changeCurrentPosition(b_is_power_on);

    b_which_stepper = !b_which_stepper;
    us_last_stepper_motion = us_current_time;
  }
}

// Public: User places this in their main loop, working off millis() it
// will adjust both arms from current position to the desired heading
// position.
void ArmSteppers::moving() {
  us_current_time = millis();
  if (isStepperEndpointMet() == false) {
    adjustDirection();
    nextStepPowered(true);
  } else {
    nextStepPowered(false);
  }
}
