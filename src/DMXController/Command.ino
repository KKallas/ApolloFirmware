void RunUartCmd(int command, String CommandString, String Params, int CurrCommandNr, int TotalCommands) {
  char printout[128]; 
  //sprintf(printout, "%i/%i, cmd: %i params: %s", CurrCommandNr, TotalCommands - 2, command, Params);
  //SendUartCmd(String(printout), true);

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
      SendUartCmd("InfoPage: [5], DMX Controller with software: [20240203] and macid: ["+MacIdString+"]", true, false);
    }
  }

  // Forward unhandeled input the device on Serial2
  SendUartCmd("DS"+CommandString+Params, true, true);
}