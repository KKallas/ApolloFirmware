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
 *   - Extracts color channel and fan intensity values from the UART data (A%i %i %i %i %i) R,G,B,W,Ttarget.
 *   - Prints the response to UART, including extracted values and current temperature.
 *   - Updates the corresponding color intensity values and fan intensity based on extracted values.
 * - For the 'I' command:
 *   - Duplicate of DMX in to test WB input with Fan (I%i %i %i %i %i) R,G,B,Wb,Ttarget
 * - For the 'T' command:
 *   - For various testing purposes
 * - For the 'M' command:
 *   - Gets lamp mac id
 */
void HandleUartCmd() {
  uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan;
  int input_brightness, wb, redin, r_in, g_in, b_in, w_in, wb_in, debug_red, debug_temp;
  int rgbw_in[4];
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
    delay(300);
    Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f,\"target_temp_input\":%fC,\"targetTempData\":%fC*\n", compRedVal, pwmValueGreen, pwmValueBlue, pwmValueWhite, pwmValueFan, currentTempData*0.125, Fan*0.125, targetTempData*0.125);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'T') {
    //Testing
    sscanf(UartReceivedChars, "T%i %i", &debug_red, &debug_temp);
    debug_temp *= 8;
    Serial.printf("\"red_in\":%i,\"temp_in\":%i,\"red_out\":%i*\n", debug_red, debug_temp, calculateRedColor(debug_red, debug_temp, true));
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'I') {
    sscanf(UartReceivedChars, "I%i %i %i %i %i %i", &r_in, &g_in, &b_in, &wb_in, &Fan);
    set_dmx(r_in, g_in, b_in, wb_in, 0);
    pwmValueRed = current_calibration_mixed[0];
    pwmValueGreen = current_calibration_mixed[1];
    pwmValueBlue = current_calibration_mixed[2];
    pwmValueWhite = current_calibration_mixed[3];
    
    // If no Fan value is provided then (codevalue 0) then use 65C
    if (Fan == 0) {
      Fan = 520;
    } else {
      targetTempData = map(Fan,1,256,30*8,80*8); // Temp range between 30-80C
    } 
    Serial.printf("\"mixed_red\":%i,\"mixed_green\":%i,\"mixed_blue\":%i,\"mixed_white\":%i*\n", current_calibration_mixed[0],current_calibration_mixed[1],current_calibration_mixed[2],current_calibration_mixed[3]);
   
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'M') {
    Serial.print("\"mac\":\"");
    Serial.print(WiFi.macAddress());
    Serial.print("\"*\n");
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D') {
    Serial.print("\"mac\":\"");
    Serial.print(WiFi.macAddress());
    Serial.print("\", ");

    Serial.printf("\"tempC\":%f,", currentTempData*0.125);
    Serial.printf("\"tempTargetC\":%f,", targetTempData*0.125);
    Serial.printf("\"fanSpeed\":%i,", pwmValueFan);

    Serial.printf("\"DmxR\":%i,", DmxData[1 + DmxOffset]);
    Serial.printf("\"DmxG\":%i,", DmxData[2 + DmxOffset]);
    Serial.printf("\"DmxB\":%i,", DmxData[3 + DmxOffset]);
    Serial.printf("\"DmxWb\":%i,", DmxData[4 + DmxOffset]);
    Serial.printf("\"DmxTempTarget\":%i,", DmxData[5 + DmxOffset]);

    Serial.printf("\"calA\":(%i, %i, %i, %i) ", current_calibration_A[0], current_calibration_A[1], current_calibration_A[2], current_calibration_A[3]);
    Serial.printf("\"calB\":(%i, %i, %i, %i) ", current_calibration_B[0],  current_calibration_B[1],  current_calibration_B[2],  current_calibration_B[3]);
    Serial.printf("\"calMixed\":(%i,%i,%i,%i)\n", current_calibration_mixed[0], current_calibration_mixed[1], current_calibration_mixed[2], current_calibration_mixed[3]);
  
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'C' && UartReceivedChars[1] == 'w') {
    sscanf(UartReceivedChars, "Cw%i %i %i %i %i %i", &wb, &input_brightness, &r_in, &g_in, &b_in, &w_in);

    // Chek WB range
    if (wb < 0 || wb > 6) {
      Serial.printf("wb too low or too high (0-6), current wb %i\n", wb);
      UartNewData = false;
      return;
    }
    // Chek Brightness range
    if (input_brightness < 0 || input_brightness > 9) {
      Serial.printf("Brightness too low or too high (0-9), current wb %i\n", wb);
      UartNewData = false;
      return;
    }

    // Write value to EEPROM
    rgbw_in[0] = r_in;
    rgbw_in[1] = g_in;
    rgbw_in[2] = b_in;
    rgbw_in[3] = w_in;
    EEPROM.put((wb*9*4*sizeof(int))+(input_brightness*4*sizeof(int)), rgbw_in);
    EEPROM.commit();

    // Update Current calibration
    calibration_points[wb][input_brightness][0] = r_in;
    calibration_points[wb][input_brightness][1] = g_in;
    calibration_points[wb][input_brightness][2] = b_in;
    calibration_points[wb][input_brightness][3] = w_in;

    Serial.printf("\"wb\":%i, \"brightness\":%i, \"r\":%i, \"g\":%i, \"b\":%i, \"w\":%i*\n", wb, input_brightness, r_in, g_in, b_in, w_in);

    UartNewData = false;
  }
    if (UartNewData == true && UartReceivedChars[0] == 'C' && UartReceivedChars[1] == 'r') {
    sscanf(UartReceivedChars, "Cr%i %i", &wb, &input_brightness);

    // Chek WB range
    if (wb < 0 || wb > 6) {
      Serial.printf("wb too low or too high (0-6), current wb %i\n", wb);
      UartNewData = false;
      return;
    }
    // Chek Brightness range
    if (input_brightness < 0 || input_brightness > 9) {
      Serial.printf("Brightness too low or too high (0-9), current wb %i\n", wb);
      UartNewData = false;
      return;
    }

    Serial.printf("\"wb\":%i, \"brightness\":%i, \"r\":%i, \"g\":%i, \"b\":%i, \"w\":%i*\n", wb, input_brightness, calibration_points[wb][input_brightness][0], calibration_points[wb][input_brightness][1], calibration_points[wb][input_brightness][2], calibration_points[wb][input_brightness][3]);
    EEPROM.get((wb*9*4*sizeof(int))+(input_brightness*4*sizeof(int)), rgbw_in);
    Serial.printf("\"EEPROM addr\":%i, \"r\":%i, \"g\":%i, \"b\":%i, \"w\":%i*\n", (wb*9*4*sizeof(int))+(input_brightness*4*sizeof(int)), rgbw_in[0], rgbw_in[1], rgbw_in[2], rgbw_in[3]);
    UartNewData = false;
  }
}
