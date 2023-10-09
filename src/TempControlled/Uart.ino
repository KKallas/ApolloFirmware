/**
 * Receives UART data and processes it to identify the end of a message.
 *
 * This function reads incoming UART characters and detects the end of a message marked by the 'endMarker' character.
 * It buffers the received characters until the end of the message is detected. Upon receiving the end of the message,
 * it marks the availability of new data by setting the 'UartNewData' flag to true.
 */
void RecvUart() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
 
  while (Serial.available() > 0 && UartNewData == false) {
    char lastChar;
    rc = Serial.read();

    if ((lastChar != '*' ) && (rc != endMarker)) {
      UartReceivedChars[ndx] = rc;
      lastChar = rc;
      ndx++;
      if (ndx >= 64) {
        ndx = 64 - 1;
      }
      Serial.print(rc);
    }
    else {
      UartReceivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      UartNewData = true;
    }
  }
}

/**
 * Handles UART commands and updates color and fan intensity values.
 *
 * This function processes the received UART data for three specific commands: 
 * - For the 'A' command:
 *   - Extracts color channel and fan intensity values from the UART data (A%i %i %i %i %i).
 *   - Prints the response to UART, including extracted values and current temperature.
 *   - Updates the corresponding color intensity values and fan intensity based on extracted values.
 * - For the 'D' command:
 *   - Sets predefined values for color channel and fan intensity.
 *   - Prints the response to UART, including predefined values and current temperature.
 *   - Updates the corresponding color intensity values and fan intensity based on predefined values.
 * - For the 'T' command:
 *   - Prints the current temperature value to UART.
 */
void HandleUartCmd() {
  uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan;
  if (UartNewData == true && UartReceivedChars[0] == 'A') {
    // Color Channels (R,G,B,W)
    sscanf(UartReceivedChars, "A%i %i %i %i %i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite, &Fan);

    // UART answer
    Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", red11bit, pwmValueGreen, pwmValueBlue, pwmValueWhite, pwmValueFan, currentTempData*0.125);
    //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

    pwmValueRed   = IntensityRed;
    pwmValueGreen = IntensityGreen;
    pwmValueBlue  = IntensityBlue;
    pwmValueWhite = IntensityWhite;
    pwmValueFan   = Fan;
    
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D') {
    pwmValueFan = updateChannel(pwmValueFan, targetValueFan, stepFan);
    Serial.printf("\"targetValueFan\":%i,\"pwmValueFan\":%i,\"stepFan\":%i*\n", targetValueFan, pwmValueFan, stepFan);
    Serial.printf("\"targetTempData\":%i,\"currentTempData\":%i*\n", targetTempData, currentTempData);
    
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'E') {
    // Temp difference, use a step for 3 seconds
    int tempDiff = targetTempData - currentTempData;
    targetValueFan = targetValueFan + tempDiff;

    stepFan = calculateStep(targetValueFan, pwmValueFan);
    pwmValueFan = updateChannel(pwmValueFan, targetValueFan, stepFan);
    Serial.printf("\"targetValueFan\":%i,\"pwmValueFan\":%i,\"stepFan\":%i*\n", targetValueFan, pwmValueFan, stepFan);
    Serial.printf("\"targetTempData\":%i,\"currentTempData\":%i,\"tempDiff\":%i*\n", targetTempData, currentTempData, tempDiff);

    
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'T') {
    Serial.printf("temp:%i\n", currentTempData);

    UartNewData = false;
  }
}