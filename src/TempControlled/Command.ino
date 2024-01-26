void RunUartCmd(int command, String Params, int CurrCommandNr, int TotalCommands) {
  char printout[128]; 
  //sprintf(printout, "%i/%i, cmd: %i params: %s", CurrCommandNr, TotalCommands - 2, command, Params);
  //SendUartCmd(String(printout), true);

  // Command 00 : GetInfo()
  if (command==0) {
    int page;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    // parse parameters
    if (sscanf(paramsCharArray, "%i", &page) != 1) {
      // TODO: value range check
      SendUartCmd("ERROR: correct input format DS00->%i", true);
      return;
    }
    // MacId
    String MacIdString = String(WiFi.macAddress());
    //MacIdString.replace(':', '|');
    
    // Temperature
    String CurrentTempString = String(currentTempData*0.125);
    String TargetTempString = String(targetTempData*0.125);
    String FanPowerString = String(pwmValueFan);
    String FanRPMString = String(fanRpm);
    String UncalibRedString = String(compRedVal);

    // Current Color
    String CurrentColorString = "RGBW("+String(pwmValueRed)+":"+String(pwmValueGreen)+":"+String(pwmValueBlue)+":"+String(pwmValueWhite)+")";

    // RGBt calibration LUT and last value
    // Using pages for simpler handling, page 0 will display all pages other will go accordignly

    String CalAString = "RGBW("+String(current_calibration_A[0])+":"+String(current_calibration_A[1])+":"+String(current_calibration_A[2])+":"+String(current_calibration_A[3])+")";
    String CalBString = "RGBW("+String(current_calibration_B[0])+":"+String(current_calibration_B[1])+":"+String(current_calibration_B[2])+":"+String(current_calibration_B[3])+")";
    String CalMixedString = "RGBW("+String(current_calibration_mixed[0])+":"+String(current_calibration_mixed[1])+":"+String(current_calibration_mixed[2])+":"+String(current_calibration_mixed[3])+")";

    if (page == 0 or page == 1) {
      SendUartCmd("InfoPage: [1], Version: [20240125], MacId: ["+MacIdString+"]",true);
    }
    if (page == 0 or page == 2) {
      SendUartCmd("InfoPage: [2], Current/TargetTemp: ["+CurrentTempString+":"+TargetTempString+"]C, FanPower(0-2048): ["+FanPowerString+"], FanSpeed: ["+FanRPMString+"]RPM, CurrentColor(0-2048): ["+CurrentColorString+"], uncalRed: ["+UncalibRedString+"]", true);
    }
    if (page == 0 or page == 3) {
      SendUartCmd("InfoPage: [3], CalA: ["+CalAString+"], CalB: ["+CalBString+"], CalMixed: ["+CalMixedString+"], last WB index: ["+String(wb_index)+"]", true);
    }
  }

  // Command 01 : SetRGBW(%i:%i:%i:%i) <- [0-2048]
  if (command==1) {
      int IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite;
      // split up params
      char paramsCharArray[64];
      Params.toCharArray(paramsCharArray, Params.length()+1);
      // parse parameters
      if (sscanf(paramsCharArray, "%i:%i:%i:%i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite) != 4) {
        // TODO: value range check
        SendUartCmd("ERROR: correct input format DS01->%i:%i:%i:%i", true);
        return;
      }
      // if new values
      sprintf(printout, "SetRGBW[%i:%i:%i:%i] <- (0-2048)", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite);
      SendUartCmd(String(printout), true);
      if ((pwmValueRed != IntensityRed) or
        (pwmValueGreen != IntensityGreen) or
        (pwmValueBlue != IntensityBlue) or
        (pwmValueWhite != IntensityWhite)) {

      // record for after boot recovery
      int packed_color[4] = {IntensityRed,IntensityGreen,IntensityBlue,IntensityWhite};
      EEPROM.put(storedLutSize+storedDmxOffsetSize, packed_color);
      EEPROM.commit();

      // set colors to dimmer
      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
    }
  }

  // Command 02: SetRGBt(%i:%i:%i:%i) <- [0-255]
  if (command==2) {
    int IntensityRed, IntensityGreen, IntensityBlue, WhiteBalance;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i:%i:%i:%i", &IntensityRed, &IntensityGreen, &IntensityBlue, &WhiteBalance) != 4) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS02->%i:%i:%i:%i", true);
      return;
    }

    sprintf(printout, "SetRGBt[%i:%i:%i:%i] <- (0-255)", IntensityRed, IntensityGreen, IntensityBlue, WhiteBalance);
    SendUartCmd(String(printout), true);

    set_RGBt(IntensityRed, IntensityGreen, IntensityBlue, WhiteBalance, 0, true);
    pwmValueRed = current_calibration_mixed[0];
    pwmValueGreen = current_calibration_mixed[1];
    pwmValueBlue = current_calibration_mixed[2];
    pwmValueWhite = current_calibration_mixed[3];
  }

  // Command 03: SetTempTarget(%i) <- [30-80C = 240-640]
  if (command==3) {
    int TargetTemp;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i", &TargetTemp) != 1) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS03->%i", true);
      return;
    }

    targetTempData = map(TargetTemp,0,255,240,640);

    sprintf(printout, "SetTempTarget(%i) [%i]/[%i]C <- |30-80C = 0-255|", TargetTemp, int(0.125*targetTempData), int(0.125*currentTempData));
    SendUartCmd(String(printout), true);
  }

  // Command 04
  if (command==4) {
    int ButtonId;
    long ButtonZeroTime;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i", &ButtonId) != 1) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS04->%i", true);
      return;
    }

    // get current time in millis
    ButtonZeroTime = millis();
    // go through the array if number is not 0 subtracti it from current time, if it is zero stop
    String AllTimes = "BTN"+String(ButtonId)+":";
    for(int i=0;i<16;i++) {
      if(ButtonPressLog[ButtonId][i]>0) {
        AllTimes = AllTimes + String(ButtonZeroTime-ButtonPressLog[ButtonId][i])+":";
      } else {
        break;
      }
    }

    // remove the last semicolon
    AllTimes = AllTimes.substring(0,AllTimes.length()-1);
    SendUartCmd(AllTimes, true);
  }

  // Command 90: SetTempCalibration(%i[%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i]) <- [0-2048]
  if (command==90) {
    int pt[11];
    int ChInput;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i[%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i]", &ChInput, &pt[0], &pt[1], &pt[2], &pt[3], &pt[4], &pt[5], &pt[6], &pt[7], &pt[8], &pt[9], &pt[10]) != 12) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS90->%i|%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i|", true);
      return;
    }

    // Set the color_calibration_points variable
    for(int i=0;i<11;i++) {
      color_calibration_points[ChInput][i] = pt[i];
    }

    // Write eeprom
    EEPROM.put(storedLutSize+storedDmxOffsetSize+storedRgbwSize, color_calibration_points);
    EEPROM.commit();
    
    sprintf(printout, "SetTempCalibration(%i[%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i]) <- (0-2048)", ChInput,
                                                                                           color_calibration_points[ChInput][0],
                                                                                           color_calibration_points[ChInput][1],
                                                                                           color_calibration_points[ChInput][2],
                                                                                           color_calibration_points[ChInput][3],
                                                                                           color_calibration_points[ChInput][4],
                                                                                           color_calibration_points[ChInput][5],
                                                                                           color_calibration_points[ChInput][6],
                                                                                           color_calibration_points[ChInput][7],
                                                                                           color_calibration_points[ChInput][8],
                                                                                           color_calibration_points[ChInput][9],
                                                                                           color_calibration_points[ChInput][10]);
    SendUartCmd(String(printout), true);
  }

  // Command 91: GetTempCalibration()
  if (command==91) {
    int ChInput;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);

    if (sscanf(paramsCharArray, "%i", &ChInput) != 1) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS91->%i ", true);
      return;
    }

    EEPROM.get(storedLutSize+storedDmxOffsetSize+storedRgbwSize, color_calibration_points);
    sprintf(printout, "GetTempCalibration(%i[%i:%i:%i:%i:%i:%i:%i:%i:%i:%i:%i]) <- (0-2048)", ChInput,
                                                                                          color_calibration_points[ChInput][0],
                                                                                          color_calibration_points[ChInput][1],
                                                                                          color_calibration_points[ChInput][2],
                                                                                          color_calibration_points[ChInput][3],
                                                                                          color_calibration_points[ChInput][4],
                                                                                          color_calibration_points[ChInput][5],
                                                                                          color_calibration_points[ChInput][6],
                                                                                          color_calibration_points[ChInput][7],
                                                                                          color_calibration_points[ChInput][8],
                                                                                          color_calibration_points[ChInput][9],
                                                                                          color_calibration_points[ChInput][10]);
    SendUartCmd(String(printout), true);
  }
  // Command 93: SetCalPoint(%i:%i[%i:%i:%i:%i:%i])
  if (command==93) {
    int RgbwLux_in[5];
    int TempInput, BrightnessInput;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);

    if (sscanf(paramsCharArray, "%i:%i:%i[%i:%i:%i:%i:%i]", &TempInput, &BrightnessInput, &RgbwLux_in[4], &RgbwLux_in[0], &RgbwLux_in[1], &RgbwLux_in[2], &RgbwLux_in[3]) != 7) {
      // TODO: Value range check
      SendUartCmd("ERROR: correct input format DS93->%i:%i:%i|%i:%i:%i:%i| KelvinTemp(0:2800,1:3200,2:4800,3:5600:4:7800,5:10000):BrightnessPoint(2-8):ActualLux|r,g,b,w 0-2048, lux|", true);
      return;
    }

    EEPROM.put((TempInput*9*5*sizeof(int))+(BrightnessInput*5*sizeof(int)), RgbwLux_in);
    EEPROM.commit();

    sprintf(printout, "WriteCalibartion %i[%i][%i:%i:%i:%i]", (TempInput*9*5*sizeof(int))+(BrightnessInput*5*sizeof(int)), RgbwLux_in[4], RgbwLux_in[0], RgbwLux_in[1], RgbwLux_in[2], RgbwLux_in[3]);
    // RgbwLux_in[4] actual lux
    SendUartCmd(String(printout), true);
  }

  // Command 94: GetCalPoint(KelvinTemp[0:2800,1:3200,2:4800,3:5600:4:7800,5:10000]:BrightnessPoint[2-8])
  if (command==94) {
    int RgbwLux_in[5];
    int TempInput, BrightnessInput;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);

    if (sscanf(paramsCharArray, "%i:%i", &TempInput, &BrightnessInput) != 2) {
      SendUartCmd("ERROR: correct input format DS94->%i:%i KelvinTemp|0:2800,1:3200,2:4800,3:5600:4:7800,5:10000|:BrightnessPoint|2-8|", true);
      return;
    }
    // TODO: Value range check


    // Read the calibration point from EEPROM
    EEPROM.get((TempInput*9*5*sizeof(int))+(BrightnessInput*5*sizeof(int)), RgbwLux_in);
    sprintf(printout, "ReadCalibartion addr:actual lux|r:g:b:W| %i:[%i][%i:%i:%i:%i]", (TempInput*9*5*sizeof(int))+(BrightnessInput*5*sizeof(int)), RgbwLux_in[4], RgbwLux_in[0], RgbwLux_in[1], RgbwLux_in[2], RgbwLux_in[3]);
    SendUartCmd(String(printout), true);
  }
  // Command 99: Send back the params
  if (command==99) {
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    SendUartCmd(String(paramsCharArray), true);
  }
}