/*void setup() {
  // Initialize serial communication
  Serial.begin(250000);

  // Other setup code
}
*/





void UartComm(void * pvParameters) {
  String taskMessage = "Running UART on: ";
  taskMessage = taskMessage + xPortGetCoreID();

  while(true) {
    processTimer1[0] = millis();
    RecvUart();
    ForwardSerial2();
    if (processTimer1[1] < millis() - processTimer1[0]) {
      processTimer1[1] = millis() - processTimer1[0];
    }
    //delayMicroseconds(50);
  }
}


// TODO: special recieve UART to get info back from the lamp
void ForwardSerial2() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  bool MoreSerial2Data = false;
  String output = "";

  while (Serial2.available() > 0 && MoreSerial2Data == false) {
    char lastChar;
    rc = Serial2.read();

    if (rc != endMarker) {
      Uart2ReceivedChars[ndx] = rc;
      lastChar = rc;
      ndx++;
    }

    if (ndx >= 512 or rc == endMarker) {
      Uart2ReceivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      MoreSerial2Data = true;
      Serial.println(Uart2ReceivedChars);
    }
  }
}

void RecvUart() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  UartNewData = false;
  while (Serial.available() > 0 && UartNewData == false) {
    char lastChar;
    rc = Serial.read();

    if (rc != endMarker) {
      Uart1ReceivedChars[ndx] = rc;
      lastChar = rc;
      ndx++;
    }
      
    // handle overflow or end marker
    if (ndx >= 512 or rc == endMarker) {
      Uart1ReceivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      UartNewData = true;
      HandleUartCmd();
    }
  }
}

void HandleUartCmd() {
  // Convert char buffer into string for easier manipulation
  String commandsInput = String(Uart1ReceivedChars);

  // find the terminator, if not found abandon package
  int terminatorLocation = commandsInput.indexOf(":::");
  if (terminatorLocation < 0) { return; }

  // Split the received message into individual commands
  String commands[16]; 
  int numCommands = splitString(Uart1ReceivedChars, "//", commands);

  // Get terminator and checksum
  String fullTerminator = commands[numCommands-1];
  int checksum = getChecksum(fullTerminator);
  if (checksum != 0) {
    int calcChecksum = calculateChecksum(commandsInput.substring(0,terminatorLocation-4));

    if (checksum != calcChecksum) {
        SendUartCmd("checksum failed packed/calculated: "+String(checksum,HEX)+"/"+String(calcChecksum,HEX), true, false);
        return;
      }
  }
  
  // Execute commands
  for (int i = 0; i < numCommands; i++) {
    if (commands[i].substring(0,2) == "DS" and commands[i].length() > 3) {
      // Split params (must start with -> or get ignored) and launch command
      String params = "";
      if (commands[i].length() > 6 and commands[i].substring(4,6) == "->") {
        params = commands[i].substring(6);
      }
      RunUartCmd(commands[i].substring(2,4).toInt(), commands[i].substring(2,6) ,params, i, numCommands);
    } 
  }
}

int unsentBufferSize = 0;
String payloads[16];

int SendUartCmd(String payload, bool sendAll, bool serial2) {
  bool send = sendAll;
  if (unsentBufferSize == 15) { send = true;}

  if(serial2) {
    payloads[unsentBufferSize] = payload + "//";
  } else {
    payloads[unsentBufferSize] = "DS10->" + payload + "//";
  }

  if (send) {
    String fullCommand;
    for(int i = 0; i < unsentBufferSize+1; i++) {
      fullCommand += payloads[i];
    }
    // create checksum
    int checksum = calculateChecksum(fullCommand);
    String hexString = String(checksum,HEX);
    while (hexString.length() < 4) {
      hexString = "0" + hexString;
    }
    if(serial2) {
      Serial2.println(fullCommand + hexString + ":::\n");
    } else {
      Serial.println(fullCommand + hexString + ":::");
    }
    unsentBufferSize = 0;
    send = false;
    return(0);
  }

  unsentBufferSize++;
  return(unsentBufferSize-1);
}


int getChecksum(String termiantor) {
  // check if the termintor contains 3 hex numbers and :::
  if(termiantor.length() != 7) { return(-1); }
  // scanf requires c style string (char array), add one more character for /0 termination
  char terminatorChar[5];
  String longTerminator = termiantor;
  longTerminator.toCharArray(terminatorChar, 5);

  int checksumVal = 0;
  sscanf(terminatorChar, "%lx", &checksumVal);
  return(checksumVal);
}

uint16_t calculateChecksum(String inputString) {
  // unit16_t to have one byte extra to handle overflow
  uint16_t checksum = 0;

  for (size_t i = 0; i < inputString.length(); i++) {
    checksum += inputString.charAt(i);

    // loop-a-round if max length achived
    if (checksum > 0xFFFF) {
      checksum -= 0xFFFF;
    }
  }

  return checksum;
}


int splitString(String inputString, const char* delimiter, String outputArray[]) {
  int i = 0;
  char* token = strtok(const_cast<char*>(inputString.c_str()), delimiter);

  while (token != NULL) {
    outputArray[i++] = String(token);
    token = strtok(NULL, delimiter);
  }

  return i;
}
