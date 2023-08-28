
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

void HandleUartCmd() {
uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan;
    if (UartNewData == true && UartReceivedChars[0] == 'A') {
      // Color Channels (R,G,B,W)
      IntensityRed   = twoByteChar(UartReceivedChars[1],UartReceivedChars[2]);
      IntensityGreen = twoByteChar(UartReceivedChars[3],UartReceivedChars[4]);
      IntensityBlue  = twoByteChar(UartReceivedChars[5],UartReceivedChars[6]);
      IntensityWhite = twoByteChar(UartReceivedChars[7],UartReceivedChars[8]);
      // Fan
      Fan = UartReceivedChars[9];

      // UART answer
      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

      pwmValueRed   = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue  = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan   = Fan;
      
      UartNewData = false;
    }
    if (UartNewData == true && UartReceivedChars[0] == 'D') {
      // Color Channels (R,G,B,W)
      //1780,1855, 410,1900
      IntensityRed   = 2000;
      IntensityGreen = 1900;
      IntensityBlue  = 500;
      IntensityWhite = 2000;
      // Fan
      Fan = 0xFF;

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);

      pwmValueRed   = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue  = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan   = Fan;
      
      UartNewData = false;
    }
    if (UartNewData == true && UartReceivedChars[0] == 'T') {
      Serial.printf("temp:%i\n", currentTempData);

      UartNewData = false;
    }
  }