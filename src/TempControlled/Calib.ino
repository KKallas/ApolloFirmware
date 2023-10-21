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
int calculateRedColor(int redColor, int temperature, bool debug) {
    // Calculation done in 16bit and output downsampled to 11bit
    // 85C on max = 2047
    // @30 on 0.66 = 1350
    // @-25 on 0.33 = 676
    const int intMaxVal = 2047;
    const int minTemp = -55 * 8;  // -55°C converted to 1/8°C
    const int maxTemp = 85 * 8;   // 85°C converted to 1/8°C

    int scale = map(temperature,-25*8,maxTemp,676,intMaxVal);
    int redColorValue = map(redColor,0,intMaxVal,0,scale);

    if(debug) {
      Serial.printf("red scale: %i, red color: %i, ", scale, redColorValue);
    }
    return redColorValue;
}



