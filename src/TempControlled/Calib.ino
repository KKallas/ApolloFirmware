/**
 * Calculate the compensated red color value based on temperature.
 *
 * This function calculates the compensated red color value for an LED based on the provided red color value and the temperature.
 * LEDs tend to output less light at higher temperatures. The function uses temperature compensation to adjust the red color value
 * proportionally to the temperature difference from a reference temperature. The compensation is performed by altering the red color
 * value based on a theoretical maximum value, the actual red color value, and the temperature offset.
 *
 * @param redColor The original red color value (11-bit).
 * @param temperature The temperature value for which compensation is calculated.
 * @return The compensated red color value after temperature compensation (11-bit).
 */
int calculateRedColor(int redColor, int temperature) {
  // Define temperature range and corresponding maxRed values
  int temperature_range[] = {-160, -80, 0, 80, 160, 240, 320, 400, 480, 560, 640};
  int maxRed_values[] = {1165, 1180, 1204, 1237, 1280, 1337, 1409, 1502, 1622, 1780, 1993};

  if (redColor == 0) {
    return 0;
  }

  if (temperature > 639) {
    int scaled_redColor = map(redColor, 0, 2048, 0, 1993);
    return scaled_redColor;
  }

  if (temperature < -159) {
    int scaled_redColor = map(redColor, 0, 2048, 0, 1165);
    return scaled_redColor;
  }

  // Find the closest lower temp point
  int closest_temperature_index = 0;
  for (int i = 1; i < 11; i++) {
    if (temperature < temperature_range[i]) {
      closest_temperature_index = i - 1; // Use the previous index
      break;
    }
  }

  // Calculate max_val based on the closest lower temperature point
  int max_val = map(temperature, temperature_range[closest_temperature_index], temperature_range[closest_temperature_index + 1], maxRed_values[closest_temperature_index], maxRed_values[closest_temperature_index + 1]);

  // Scale the redColor using map
  int scaled_redColor = map(redColor, 0, 2048, 0, max_val);
  if (scaled_redColor == 0) {
    scaled_redColor = 1;
  }
  return scaled_redColor;
}








