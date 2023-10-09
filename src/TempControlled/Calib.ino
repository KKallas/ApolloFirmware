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
int calculateRedColor(int redColor, int temperature) {
    // Calculation done in 16bit and output downsampled to 11bit
    // 85C on max = 32767
    // @30 on 0.66 = 21615
    // @-25 on 0.33 = 10816
    int tempOffset = 85-temperature;
    int tempCompen = 32767-(tempOffset*198);

    // lookat map

    // Adjust the red color value based on the provided redColor parameter
    int redColorValue = (tempCompen * redColor); // 16bit theorticalMax*actualValue
    redColorValue = redColorValue >> 16;                 // devide by 65K to normalize the answer
    //redColorValue = redColorValue >> 5;                  // downscale to 11bits
    return (int)redColorValue;
}

