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
 *   - For various testing purposes
 */
void HandleUartCmd() {
  uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan;
  int input_brightness, wb, redin, r_in, g_in, b_in, wb_in;
  if (UartNewData == true && UartReceivedChars[0] == 'A') {
    // Color Channels (R,G,B,W, tempTarget)
    sscanf(UartReceivedChars, "A%i %i %i %i %i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite, &Fan);

    pwmValueRed    = IntensityRed;
    pwmValueGreen  = IntensityGreen;
    pwmValueBlue   = IntensityBlue;
    pwmValueWhite  = IntensityWhite;

    // If no Fan value is provided then (codevalue 0) then use 65C
    if (Fan == 0) {
      Fan = 520;
    } else {
      targetTempData = map(Fan,1,256,30*8,80*8); // Temp range between 30-80C
    }    
  
    // UART answer
    Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f,\"target_temp_input\":%fC,\"targetTempData\":%fC*\n", compRedVal, pwmValueGreen, pwmValueBlue, pwmValueWhite, pwmValueFan, currentTempData*0.125, Fan*0.125, targetTempData*0.125);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D') {
    Serial.printf("\"targetTempData\":%fC,\"currentTempData\":%fC,\"pwmValueFan\":%i,\"stepFan\":%i*\n", targetTempData*0.125, currentTempData*0.125, pwmValueFan, FanSpeedStep);
    
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'T') {
    // Brightness
    sscanf(UartReceivedChars, "T%i %i", &input_brightness, &wb);
    update_calib(input_brightness, wb);
    pwmValueRed = current_calibration_mixed[0];
    pwmValueGreen = current_calibration_mixed[1];
    pwmValueBlue = current_calibration_mixed[2];
    pwmValueWhite = current_calibration_mixed[3];

    Serial.printf("\"red\":%i,\"green\":%i,\"blue\":%i,\"white\":%i*\n", current_calibration_A[0],current_calibration_A[1],current_calibration_A[2],current_calibration_A[3]);
    Serial.printf("\"mixed_red\":%i,\"mixed_green\":%i,\"mixed_blue\":%i,\"mixed_white\":%i*\n", current_calibration_mixed[0],current_calibration_mixed[1],current_calibration_mixed[2],current_calibration_mixed[3]);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'R') {
    sscanf(UartReceivedChars, "R%i", &redin);
    calculateRedColor(redin, currentTempData>>3, true);
    Serial.printf("current temp data: %i\n", currentTempData>>3);

    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'I') {
    sscanf(UartReceivedChars, "I%i %i %i %i", &r_in, &g_in, &b_in, &wb_in);
    set_dmx(r_in, g_in, b_in, wb_in, 0);
    pwmValueRed = current_calibration_mixed[0];
    pwmValueGreen = current_calibration_mixed[1];
    pwmValueBlue = current_calibration_mixed[2];
    pwmValueWhite = current_calibration_mixed[3];
    Serial.printf("\"mixed_red\":%i,\"mixed_green\":%i,\"mixed_blue\":%i,\"mixed_white\":%i*\n", current_calibration_mixed[0],current_calibration_mixed[1],current_calibration_mixed[2],current_calibration_mixed[3]);
   
    UartNewData = false;
  }
}