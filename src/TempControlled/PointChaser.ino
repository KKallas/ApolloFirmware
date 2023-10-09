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
  int step = (current - target) / STEPS;
  return step;
}

/**
 * Update the current value towards a target value with a specified step.
 *
 * This function updates the current value towards a target value by applying a specified step. It ensures
 * that the current value approaches the target value by modifying the current value in increments of the step.
 * The step can be positive or negative, determining the direction of the update. The function handles cases
 * where the step would overshoot the target value, ensuring that the current value reaches the target value
 * without exceeding it.
 *
 * @param currentValue The current value to be updated.
 * @param targetValue The target value to which the current value should be updated.
 * @param step The step value that determines the rate of change towards the target value.
 * @return The updated current value after applying the step.
 */
int updateChannel(int currentValue, int targetValue, int step) {
  if (step == 0) {
    return(currentValue);
  }
  int nextValue = currentValue + step;
  if ((step < 0) and (nextValue <= 0)) {
      step = 0;
      return 0;
  }
  if ((step > 0) and (nextValue >= targetValue)) {
      step = 0;
      return targetValue;
  }
  
  return(nextValue);
}
