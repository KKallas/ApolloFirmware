
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