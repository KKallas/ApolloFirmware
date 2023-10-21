// TempTarget if smaller then slow fan down a bit if higher speed fan up a bit once every 10 seconds?
// I should split this into steps just like the interpüolated output
void updateFanSpeed() {
  int diff = currentTempData - targetTempData;

  // add or emove a step
  if (diff < (1*8)) {                 // 1C is 8 codevalues
    pwmValueFan = pwmValueFan - FanSpeedStep;
  }
  if (diff > (1*8)) {                 // 1C is 8 codevalues
    pwmValueFan = pwmValueFan + FanSpeedStep;
  }

  // clip the limits
  if (pwmValueFan < 0) {
    pwmValueFan = 0;
  }
  if (pwmValueFan > 2047) {
    pwmValueFan = 2047;
  }
}

