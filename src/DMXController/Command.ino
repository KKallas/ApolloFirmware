void RunUartCmd(int command, String CommandString, String Params, int CurrCommandNr, int TotalCommands) {
  char printout[128]; 
  //sprintf(printout, "%i/%i, cmd: %i params: %s", CurrCommandNr, TotalCommands - 2, command, Params);
  //SendUartCmd(String(printout), true);

  // Command 05: dmxOffset [0-511]
  if (command == 5) {
    int readValue = -1;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i", &readValue) != 1) {
      // TODO: value range check
      SendUartCmd("ERROR: correct input format DS05->%i(0-511)", true, false);
      return;
    }
    dmxOffset = readValue;
  }

    // Command 06: dmxEnabled [0-1]
  if (command == 6) {
    int readValue = -1;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    if (sscanf(paramsCharArray, "%i", &readValue) != 1) {
      // TODO: value range check
      SendUartCmd("ERROR: correct input format DS06->%i(0-1)", true, false);
      return;
    }
    if(readValue == 0) {
      dmxEnabled = false;
    } else {
      dmxEnabled = true;
    }
  }

  // Command 99: Send back the params
  if (command==99) {
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    SendUartCmd(String(paramsCharArray), true, false);
    return;
  }

  if (command==0) {
    int page;
    // split up params
    char paramsCharArray[64];
    Params.toCharArray(paramsCharArray, Params.length()+1);
    // parse parameters
    if (sscanf(paramsCharArray, "%i", &page) != 1) {
      // TODO: value range check
      SendUartCmd("ERROR: correct input format DS00->%i", true, false);
      return;
    }
    // MacId
    String MacIdString = String(WiFi.macAddress());

    if (page == 0 or page == 5) {
      delayMicroseconds(10000);
      SendUartCmd("InfoPage: [5], DMX Controller with software: [20240219] and macid: ["+MacIdString+"]", true, false);
    }
    if (page == 0 or page == 6) {
      delayMicroseconds(10000);
      if(dmxEnabled) {
        SendUartCmd("InfoPage: [6], DMX offset: [" + String(dmxOffset) + "] and dmx is: [1] - enabled", true, false);
      } else {
        SendUartCmd("InfoPage: [6], DMX offset: [" + String(dmxOffset) + "] and dmx is: [0] - disabled", true, false);
      }
      // Collect data for I2C Rotary encoder
      lastValI2C[5] = dmxOffset;
      lastValI2C[6] = dmxEnabled;
    }
  }

  // Forward unhandeled input the device on Serial2
  SendUartCmd("DS"+CommandString+Params, true, true);
}