void ColorUpdate(void * pvParameters){
 
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

    ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(1, pwmFrequency, pwmResolution); 
    ledcSetup(2, pwmFrequency, pwmResolution); 
    ledcSetup(3, pwmFrequency, pwmResolution); 
    ledcSetup(4, pwmFrequency, pwmResolution); 
    ledcAttachPin(redPin, 0);
    ledcAttachPin(greenPin, 1);
    ledcAttachPin(bluePin, 2);
    ledcAttachPin(whitePin, 3);
    ledcAttachPin(fanPin, 4);

    while(true){
      // Overheting protection/Red comp disbale option 
      if (overheated == false) {    

        // Red comp with disable option, all other than redcalib should set disable_red_comp = false;
        if (disable_red_comp==true) {
          compRedVal = pwmValueRed;
        } else {
          compRedVal = calculateRedColor(pwmValueRed, currentTempData);
        }

        if (powerSate) {
          ledcWrite(0, compRedVal);
          ledcWrite(1, pwmValueGreen);
          ledcWrite(2, pwmValueBlue);
          ledcWrite(3, pwmValueWhite);
        } else {
          ledcWrite(0, 0);
          ledcWrite(1, 0);
          ledcWrite(2, 0);
          ledcWrite(3, 0);
        }
        ledcWrite(4, pwmValueFan);
      } else {
        ledcWrite(0, 0);
        ledcWrite(1, 0);
        ledcWrite(2, 0);
        ledcWrite(3, 0);
        ledcWrite(4, 2047);
      }
      // Wait for next update
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}


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

  if (redColor == 0) {
    return 0;
  }

  if (temperature > 639) {
    int scaled_redColor = map(redColor, 0, 2048, 0, color_calibration_points[0][10]);
    return scaled_redColor;
  }

  if (temperature < -159) {
    int scaled_redColor = map(redColor, 0, 2048, 0, color_calibration_points[0][0]);
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
  int max_val = map(temperature, temperature_range[closest_temperature_index], temperature_range[closest_temperature_index + 1], color_calibration_points[0][closest_temperature_index], color_calibration_points[0][closest_temperature_index + 1]);
  //Serial.printf("%i and %i -> %i:%i\n", color_calibration_points[0][closest_temperature_index], color_calibration_points[0][closest_temperature_index + 1],max_val,closest_temperature_index);
  // Scale the redColor using map
  int scaled_redColor = map(redColor, 0, 2048, 0, max_val);
  if (scaled_redColor == 0) {
    scaled_redColor = 1;
  }
  return scaled_redColor;
}

/**
 * Update LED Lamp Calibration
 *

 * This function converts the brightness of an RGBt component into lamp-specific RGBW values using a Look-Up Table (LUT)
 * stored in the EEPROM. The LUT is organized with key points for White Balance (WB) at [2800, 3200, 4800, 5600, 7800, 10000] Kelvin values, 
 * and brightness levels ranging from minimum to maximum in 2x lux increments.
 * [min]->[0, 1, 2, 3, 4, 5, 6, 7, 8]->[max] (maximum values according to whitebalance [2800-3200] corresponds to 10,000 lux, [4800-10000] to 20,000 lux).
 * 
 * The Kelvin value is mixed by selecting two RGBW values from the LUT closest to the 8-bit input (Cal A, Cal B, and Cal Mixed).
 * The white value is determined within a one-calibration-Kelvin range, based on the two closest brightness points to the input intensity.
 * 
 * 2800-3200-4800-5600-7800-10000 = 7200K (full range)
 * 0    400  2000 2800 5000 7200          (offset from 2800)
 * 0    14   71   100  178  255           (offset as 8 bit values for DMX)
 * 
 * @param intensity The intensity of the input (0-255).
 * @param wb The white balance input (0-255).
 */

void update_calib(int intensity, int wb) {
  // Find the index for the last smaller intensity value in the lookup table
  int int_index = findLastSmallerInt(intensity);

  // Find the index for the last smaller white balance value in the lookup table     
  wb_index = findLastSmallerWb(wb);

  // Handle a special case when white balance is 0
  if (wb==0) {
    wb_index=0;
  }

  // Calibration A
  for(int i = 0; i < 4; i++) {
    // Interpolate output values based on intensity and white balance for Cal A
    current_calibration_A[i] = map(intensity, intesity_list[int_index], intesity_list[int_index + 1], calibration_points[wb_index][int_index][i], calibration_points[wb_index][int_index + 1][i]);
  }

  // Handle edge cases when intensity is at its minimum or maximum
  if (intensity == 0) {
    // Set values to the minimum calibration point for Calibration A
    current_calibration_A[0] = calibration_points[wb_index][0][0];
    current_calibration_A[1] = calibration_points[wb_index][0][1];
    current_calibration_A[2] = calibration_points[wb_index][0][2];
    current_calibration_A[3] = calibration_points[wb_index][0][3];
  }
  if (intensity == 255) {
    // Set values to the maximum calibration point for Calibration A
    current_calibration_A[0] = calibration_points[wb_index][8][0];
    current_calibration_A[1] = calibration_points[wb_index][8][1];
    current_calibration_A[2] = calibration_points[wb_index][8][2];
    current_calibration_A[3] = calibration_points[wb_index][8][3];
  }

  // Calibration B
  wb_index = wb_index + 1;

  // Ensure that wb_index does not exceed the maximum index 
  if (wb_index > 5) {
    wb_index = 5;
  }

  for(int i = 0; i < 4; i++) {
    // Interpolate output values based on intensity and white balance for Cal B
    current_calibration_B[i] = map(intensity, intesity_list[int_index], intesity_list[int_index + 1], calibration_points[wb_index][int_index][i], calibration_points[wb_index][int_index + 1][i]);
  }

  // Handle edge cases when intensity is at its minimum or maximum for Calibration B
  if (intensity == 0) {
    // Set values to the minimum calibration point for Calibration B
    current_calibration_B[0] = calibration_points[wb_index][0][0];
    current_calibration_B[1] = calibration_points[wb_index][0][1];
    current_calibration_B[2] = calibration_points[wb_index][0][2];
    current_calibration_B[3] = calibration_points[wb_index][0][3];
  }
  if (intensity == 255) {
    // Set values to the maximum calibration point for Calibration B
    current_calibration_B[0] = calibration_points[wb_index][8][0];
    current_calibration_B[1] = calibration_points[wb_index][8][1];
    current_calibration_B[2] = calibration_points[wb_index][8][2];
    current_calibration_B[3] = calibration_points[wb_index][8][3];
  }

  // Calculate mixed calibration based on white balance
  current_calibration_mixed[0] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[0],current_calibration_B[0]);
  current_calibration_mixed[1] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[1],current_calibration_B[1]);
  current_calibration_mixed[2] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[2],current_calibration_B[2]);
  current_calibration_mixed[3] = map(wb,wb_index_list[wb_index-1],wb_index_list[wb_index],current_calibration_A[3],current_calibration_B[3]);
}

/**
 * Find the Last Smaller Intensity Index
 *
 * This function searches for the index in the intensity list where the provided input intensity value
 * is greater than the corresponding intensity list value. It returns the last index where this condition
 * is met, effectively finding the largest intensity that is smaller than the input value.
 *
 * @param inputIntensity The input intensity value to search for.
 * @return The index in the intensity list where the input intensity is greater than the list value.
 */
int findLastSmallerInt(int inputIntensity) {
  int lastIndex = -1;  // Initialize with an invalid index

  for (int i = 0; i < 9; i++) {
    if (inputIntensity > intesity_list[i]) {
      lastIndex = i;  // Update the last index if the condition is met
    }
  }

  return lastIndex;
}

/**
 * Find the Last Smaller White Balance Index
 *
 * This function searches for the index in the white balance index list where the provided input white balance value
 * is greater than the corresponding white balance list value. It returns the last index where this condition is met,
 * effectively finding the largest white balance value that is smaller than the input value.
 *
 * @param inputWb The input white balance value to search for.
 * @return The index in the white balance index list where the input white balance is greater than the list value.
 */
int findLastSmallerWb(int inputWb) {
  int lastIndex = -1;  // Initialize with an invalid index

  for (int i = 0; i < 6; i++) {
    if (inputWb > wb_index_list[i]) {
      lastIndex = i;  // Update the last index if the condition is met
    }
  }

  return lastIndex;
}


void set_RGBt(int r_in, int g_in, int b_in, int wb_in, int temp_in, bool save) {
  if ((r_in==0) && (g_in==0) && (b_in==0)) {
    current_calibration_mixed[0] = 0;
    current_calibration_mixed[1] = 0;
    current_calibration_mixed[2] = 0;
    current_calibration_mixed[3] = 0;
    return;
  }
  // Make sure that red compensation is enabled, otherwise red output will drift based on lamp temperature
  disable_red_comp = false;
  // Initialize lowest_value with r_in initially
  int lowest_value = r_in;

  // Compare with g_in
  if (g_in < lowest_value) {
    lowest_value = g_in;
  }

  // Compare with b_in
  if (b_in < lowest_value) {
    lowest_value = b_in;
  }

  update_calib(lowest_value, wb_in);
  // Mix in the color
  // TODO: convert the RGB gamma, use LUT instead of multiply
  current_calibration_mixed[0] = (r_in - lowest_value) * 8 + current_calibration_mixed[0];
  current_calibration_mixed[1] = (g_in - lowest_value) * 8 + current_calibration_mixed[1];
  current_calibration_mixed[2] = (b_in - lowest_value) * 8 + current_calibration_mixed[2];

  // only if new values
  if(save) {
    EEPROM.put(storedLutSize+storedDmxOffsetSize, current_calibration_mixed);
    EEPROM.commit();
  }
}