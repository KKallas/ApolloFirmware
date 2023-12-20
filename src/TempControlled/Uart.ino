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
      //Serial.print(rc);
    }
    else {
      UartReceivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      UartNewData = true;
    }
  }
}

// TODO: needs update
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
  uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite;
  int input_brightness, wb, redin, r_in, g_in, b_in, w_in, wb_in, lx_in, debug_red, debug_temp, lamp_temp, dmx_in, ch_in;
  int color_ch[11];
  int rgbwx_in[5];
  
  if (UartNewData == true && UartReceivedChars[0] == 'A') {
    // Color Channels (R,G,B,W, tempTarget)
    sscanf(UartReceivedChars, "A%i %i %i %i %i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite, &lamp_temp);

    // If no Fan value is provided then (codevalue 0) then use 65C
    if (lamp_temp == 0) {
      targetTempData = 520;
    } else {
      targetTempData = map(lamp_temp,1,256,30*8,80*8); // Temp range between 30-80C
    }    
  
    // UART store last color
    if ((pwmValueRed != IntensityRed) or
        (pwmValueGreen != IntensityGreen) or
        (pwmValueBlue != IntensityBlue) or
        (pwmValueWhite != IntensityWhite)) {

      int packed_color[4] = {IntensityRed,IntensityGreen,IntensityBlue,IntensityWhite};
      EEPROM.put(storedLutSize+storedDmxOffsetSize, packed_color);
      EEPROM.commit();

      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
    }

    delay(300);
    Serial.printf("\"red_val\":%i,\"uncalibrated_red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f,\"target_temp_inputC\":%f,\"targetTempDataC\":%f,\"fanRPM\":%i*\n", compRedVal, pwmValueRed, pwmValueGreen, pwmValueBlue, pwmValueWhite, pwmValueFan, currentTempData*0.125, lamp_temp*0.125, targetTempData*0.125, fanRpm);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'T') {
    //Testing
    sscanf(UartReceivedChars, "T%i %i", &debug_red, &debug_temp);
    debug_temp *= 8;
    Serial.printf("\"red_in\":%i,\"temp_in\":%i,\"red_out\":%i*\n", debug_red, debug_temp, calculateRedColor(debug_red, debug_temp));
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'I') {
    if (programSate && powerSate) {
      sscanf(UartReceivedChars, "I%i %i %i %i %i %i", &r_in, &g_in, &b_in, &wb_in, &lamp_temp);
      set_dmx(r_in, g_in, b_in, wb_in, 0, true);
      pwmValueRed = current_calibration_mixed[0];
      pwmValueGreen = current_calibration_mixed[1];
      pwmValueBlue = current_calibration_mixed[2];
      pwmValueWhite = current_calibration_mixed[3];
      
      // If no Fan value is provided then (codevalue 0) then use 65C
      if (lamp_temp == 0) {
        targetTempData = 520;
      } else {
        targetTempData = map(lamp_temp,0,255,240,640); // Temp range between 30-80C
      } 
      Serial.printf("\"mixed_red\":%i,\"mixed_green\":%i,\"mixed_blue\":%i,\"mixed_white\":%i,\"red_actual\":%i,\"lamp_temp\":%f,\"lamp_target\":%f,\"fan_power\":%i,\"fan_rpm\":%i*\n", current_calibration_mixed[0],current_calibration_mixed[1],current_calibration_mixed[2],current_calibration_mixed[3],compRedVal,currentTempData*0.125, targetTempData*0.125, pwmValueFan, fanRpm);
    } else {
      Serial.print("\"error\":\"Could not set new color value as lamp is not in Program or On mode\"*\n");
    }
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'K') {
    // Color Channels (R,G,B,W, tempTarget)
    sscanf(UartReceivedChars, "K%i %i %i %i %i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite, &lamp_temp);

    // If no Fan value is provided then (codevalue 0) then use 65C
    if (lamp_temp == 0) {
      targetTempData = 520;
    } else {
      targetTempData = map(lamp_temp,1,256,30*8,80*8); // Temp range between 30-80C
    }    
    // If no Red value is provided, then turn Red compensation on again
    if (IntensityRed == 0) {
      disable_red_comp = false;  // red compensation on again
    } else {
      disable_red_comp = true;  // red compensation off
    }    
    pwmValueRed = IntensityRed;
    pwmValueGreen = IntensityGreen;
    pwmValueBlue = IntensityBlue;
    pwmValueWhite = IntensityWhite;
  
    delay(300);
    Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f,\"target_temp_inputC\":%f,\"targetTempDataC\":%f,\"fanRPM\":%i*\n", pwmValueRed, pwmValueGreen, pwmValueBlue, pwmValueWhite, pwmValueFan, currentTempData*0.125, lamp_temp*0.125, targetTempData*0.125, fanRpm);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'M') {
    Serial.print("\"mac\":\"");
    Serial.print(WiFi.macAddress());
    Serial.print("\"*\n");
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D' && UartReceivedChars[1] == ' ') {
    Serial.print("\"mac\":\"");
    Serial.print(WiFi.macAddress());
    Serial.print("\", ");

    Serial.print("\"Version\":20231220,"); //reversed europen date

    Serial.printf("\"tempC\":%f,", currentTempData*0.125);
    Serial.printf("\"tempTargetC\":%f,", targetTempData*0.125);
    Serial.printf("\"fanPower\":%i,", pwmValueFan);
    Serial.printf("\"fanRPM\":%i,", fanRpm);

    Serial.printf("\"DmxR\":%i,", DmxData[1 + DmxOffset]);
    Serial.printf("\"DmxG\":%i,", DmxData[2 + DmxOffset]);
    Serial.printf("\"DmxB\":%i,", DmxData[3 + DmxOffset]);
    Serial.printf("\"DmxWb\":%i,", DmxData[4 + DmxOffset]);
    Serial.printf("\"DmxTempTarget\":%i,", DmxData[5 + DmxOffset]);

    Serial.printf("\"calA\":(%i, %i, %i, %i), ", current_calibration_A[0], current_calibration_A[1], current_calibration_A[2], current_calibration_A[3]);
    Serial.printf("\"calB\":(%i, %i, %i, %i), ", current_calibration_B[0],  current_calibration_B[1],  current_calibration_B[2],  current_calibration_B[3]);
    Serial.printf("\"calMixed\":(%i,%i,%i,%i)\n", current_calibration_mixed[0], current_calibration_mixed[1], current_calibration_mixed[2], current_calibration_mixed[3]);
  
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D' && UartReceivedChars[1] == 'd') {
    sscanf(UartReceivedChars, "Dd%i ",&dmx_in);
    if(dmx_in==0) {
      dmxDebugState = false;
    } else {
      dmxDebugState = true;
    }
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D' && UartReceivedChars[1] == 'e') {
    sscanf(UartReceivedChars, "De%i ",&dmx_in);
    if(dmx_in==0) {
      Serial.print("\"dmx_enabled\":0*\n");
      dmx_enable = false;
    } else {
      Serial.print("\"dmx_enabled\":1*\n");
      dmx_enable = true;
    }
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D' && UartReceivedChars[1] == 'r') {
    Serial.printf("\"dmx_offset\":%i \n", DmxOffset);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'D' && UartReceivedChars[1] == 'w') {
    sscanf(UartReceivedChars, "Dw %d", &dmx_in);
    DmxOffset = dmx_in;
    
    // Clip input values
    if (dmx_in < 0) {
      dmx_in = 0;
    }
    if (dmx_in > 511) {
      dmx_in = 511;
    }

    Serial.printf("\"dmx_offset\":%i \n",DmxOffset);
    EEPROM.put(storedLutSize+1,dmx_in);
    EEPROM.commit();

    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'R' && UartReceivedChars[1] == 'r') {
    sscanf(UartReceivedChars, "Rr %i", &ch_in);
    
    // Check if channels are [0-3]
    if((ch_in < 0) or (ch_in > 3)) {
      Serial.print("\"error\":\"Channel must be 0-3 [RGBW]\"*\n");
      UartNewData = false;
      return;
    }

    Serial.printf("\"ch\":%i, \"current_lut\":{%i, %i, %i, %i, %i, :%i, %i, %i, %i, %i, %i}*\n", ch_in, 
                                                                                              color_calibration_points[ch_in][0],
                                                                                              color_calibration_points[ch_in][1],
                                                                                              color_calibration_points[ch_in][2],
                                                                                              color_calibration_points[ch_in][3],
                                                                                              color_calibration_points[ch_in][4],
                                                                                              color_calibration_points[ch_in][5],
                                                                                              color_calibration_points[ch_in][6],
                                                                                              color_calibration_points[ch_in][7],
                                                                                              color_calibration_points[ch_in][8],
                                                                                              color_calibration_points[ch_in][9],
                                                                                              color_calibration_points[ch_in][10]
                                                                                              );
    EEPROM.get(storedLutSize+storedDmxOffsetSize+storedRgbwSize, color_calibration_points);
    Serial.printf("\"EEPROM addr\":%i, \"ch\":%i, \"current_lut\":{%i, :%i, %i, %i, %i, :%i, %i, %i, %i, %i, %i}*\n", storedLutSize+storedDmxOffsetSize+storedRgbwSize,
                                                                                                                  ch_in,
                                                                                                                  color_calibration_points[ch_in][0],
                                                                                                                  color_calibration_points[ch_in][1],
                                                                                                                  color_calibration_points[ch_in][2],
                                                                                                                  color_calibration_points[ch_in][3],
                                                                                                                  color_calibration_points[ch_in][4],
                                                                                                                  color_calibration_points[ch_in][5],
                                                                                                                  color_calibration_points[ch_in][6],
                                                                                                                  color_calibration_points[ch_in][7],
                                                                                                                  color_calibration_points[ch_in][8],
                                                                                                                  color_calibration_points[ch_in][9],
                                                                                                                  color_calibration_points[ch_in][10]
                                                                                                                  );
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'R' && UartReceivedChars[1] == 'w') {
    sscanf(UartReceivedChars, "Rw %i %i %i %i %i %i %i %i %i %i %i %i", &ch_in, &color_ch[0], &color_ch[1], &color_ch[2], &color_ch[3], &color_ch[4], &color_ch[5], &color_ch[6], &color_ch[7], &color_ch[8], &color_ch[9], &color_ch[10]);

    // Check if channels are [0-3] and all values are within 1-2048 range
    if((ch_in < 0) or (ch_in > 3)) {
      Serial.print("\"error\":\"Channel must be 0-3 [RGBW]\"*\n");
      UartNewData = false;
      return;
    }
    for(int i = 0; i < 11; i++) {
      if((color_ch[i] > 2048) or (color_ch[i] < 1 )) {
        Serial.print("\"error\":\"All Red LUT values must be from 1-2048\"*\n");
        UartNewData = false;
        return;
      }
    }

    // Update Current calibration
    Serial.printf("\"ch\":%i, {", ch_in);
    for(int i=0; i<11;i++) {
      color_calibration_points[ch_in][i] = color_ch[i];
      Serial.printf("(\"temp\":%f, \"output_scale\": %i), ", temperature_range[i]*0.125, color_calibration_points[ch_in][i]);
    }
    Serial.printf("\"write_addr\": %i*\n",storedLutSize+storedDmxOffsetSize+storedRgbwSize);

    // Write value to EEPROM
    EEPROM.put(storedLutSize+storedDmxOffsetSize+storedRgbwSize, color_calibration_points);
    EEPROM.commit();

    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'C' && UartReceivedChars[1] == 'w') {
    sscanf(UartReceivedChars, "Cw %i %i %i %i %i %i %i", &wb, &input_brightness, &lx_in, &r_in, &g_in, &b_in, &w_in);

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
    // Check lux val
    if (lx_in < 0 || lx_in > 65000) {
      Serial.printf("Lux input: %i must be from 0-65000\n", lx_in);
      UartNewData = false;
      return;
    }

    // Write value to EEPROM
    rgbwx_in[0] = r_in;
    rgbwx_in[1] = g_in;
    rgbwx_in[2] = b_in;
    rgbwx_in[3] = w_in;
    rgbwx_in[4] = lx_in;
    EEPROM.put((wb*9*5*sizeof(int))+(input_brightness*5*sizeof(int)), rgbwx_in);
    EEPROM.commit();

    // Update Current calibration
    calibration_points[wb][input_brightness][0] = r_in;
    calibration_points[wb][input_brightness][1] = g_in;
    calibration_points[wb][input_brightness][2] = b_in;
    calibration_points[wb][input_brightness][3] = w_in;

    Serial.printf("\"wb\":%i, \"brightness\":%i, \"lux\":%i, \"r\":%i, \"g\":%i, \"b\":%i, \"w\":%i*\n", wb, input_brightness, lx_in, r_in, g_in, b_in, w_in);

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
    EEPROM.get((wb*9*5*sizeof(int))+(input_brightness*5*sizeof(int)), rgbwx_in);
    Serial.printf("\"EEPROM addr\":%i, \"lux\":%i, \"r\":%i, \"g\":%i, \"b\":%i, \"w\":%i*\n", (wb*9*5*sizeof(int))+(input_brightness*5*sizeof(int)), rgbwx_in[4], rgbwx_in[0], rgbwx_in[1], rgbwx_in[2], rgbwx_in[3]);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'R') {
    disable_red_comp = true;                       // once calling this command it is possible to use A command without red compensation until restart
    sscanf(UartReceivedChars, "R %i", &lamp_temp); // user will ask for 1C increment temp, if temp is not correct then target temp or RGBW value is set, if correct settings are kept and max red is switched on
    targetTempData = lamp_temp*8;

    // if the lamp is within half a Celsius, measuremeant can be taken
    if (abs(targetTempData - currentTempData) > 4) {
      if ((targetTempData - currentTempData) > 0) {
        // Use max RGBW to heat the lamp
        pwmValueFan = 0;
        pwmValueRed = 2048;
        pwmValueGreen = 1024;
        pwmValueBlue = 1024;
        pwmValueWhite = 2048;
      }
      else
      {
        // Let the fan take over just switch off the LEDs
        pwmValueFan = 2048;
        pwmValueRed = 10;
        pwmValueGreen = 10;
        pwmValueBlue = 10;
        pwmValueWhite = 10;
      }
      Serial.printf("\"ready\":False, \"tempC\":%f,\"tempTargetC\":%f,\"fanSpeed\":%i \n",currentTempData*0.125,targetTempData*0.125,pwmValueFan);
    } else {
      pwmValueFan = 0;
      pwmValueRed = 2048;
      pwmValueGreen = 0;
      pwmValueBlue = 0;
      pwmValueWhite = 0;
      Serial.printf("\"ready\":True, \"tempC\":%f,\"tempTargetC\":%f,\"fanSpeed\":%i \n",currentTempData*0.125,targetTempData*0.125,pwmValueFan);
    }
    UartNewData = false;
  }
  if (UartNewData == true) {
    Serial.print("Unkown command...\n");
    UartNewData = false;
  }
}
