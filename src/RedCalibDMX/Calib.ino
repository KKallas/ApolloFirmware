/**
 * Calculate the compensated red color value based on temperature.
 *
 * This function calculates the compensated red color value for an LED based on the provided red color value and the temperature.
 * LEDs tend to output less light at higher temperatures. The function uses temperature compensation to adjust the red color value
 * proportionally to the temperature difference from a reference temperature. The compensation is performed by altering the red color
 * value based on a theoretical maximum value, the actual red color value, and the temperature offset.
 *
 * @param redColor The original red color value (16-bit) to be compensated.
 * @param temperature The temperature value for which compensation is calculated.
 * @return The compensated red color value after temperature compensation (11-bit).
 */
int calculateRedColor(uint redColor, uint temperature) {
    // Calculation on 16bit out 11bit
    // 85C on max = 2047
    // @30 on 0.66 = 1351
    // @-25 on 0.33 = 676
    uint tempOffset = 85-temperature;
    uint tempCompen = 65535-(tempOffset*395);

    // lookat map

    // Adjust the red color value based on the provided redColor parameter
    uint redColorValue = (tempCompen * (redColor << 5)); // 16bit theorticalMax*actualValue
    redColorValue = redColorValue >> 16;                 // devide by 65K to normalize the answer
    redColorValue = redColorValue >> 5;                  // downscale to 11bits
    return (uint)redColorValue;
}

/**
 * Calculate the step value needed to approach a target value from a current value.
 *
 * This function is used calculate the step value required to approach a target value from a current value in a specified
 * number of steps. The direction of the step is determined by whether the current value is greater or less than
 * the target value. The calculated step is based on dividing the difference between the current value and the target
 * value by the specified number of steps. 
 *
 * @param target The target value to which the current value should approach.
 * @param current The current value from which the approach should begin.
 * @return The calculated step value to approach the target value in the specified number of steps.
 */
int calculateStep(int target, int current) {
  int step = 0;
  if (current > target) {
    step = (current - target) / STEPS;
  } else {
    step = (target - current) / STEPS;
    step = -step;
  }
  return step;
}