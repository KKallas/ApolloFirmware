void receiveEvent(int howMany) {
  int i = 0;
  while (Wire.available()) {
    int partA = Wire.read();
    int partB = Wire.read();
    recievedData[i] = partA + (partB * 256);
    i++;
  }
  SendUartCmd("DS02->"+String(recievedData[2])+":"+String(recievedData[3])+":"+String(recievedData[4])+":"+String(convertKelvinToByte(recievedData[1])), true, true);
  //Serial.printf("DEBUG: %i:%i\n", recievedData[1], convertKelvinToByte(recievedData[1]));
  // Get DMX enabled fom GUI
  dmxEnabled = recievedData[6];
  lastValI2C[6] = dmxEnabled;
  // TODO: Add fan and temp data
}

void requestEvent() {
  for(int i = 0; i < 9; i++) {
    Wire.write((byte *)&lastValI2C[i],2); // Send the current color back to the master
  }
}

// TODO make curve implementations
/**
 * Convert Kelvin To Byte
 *
 * Take 2800-10000 input and make it into 8 bit output according to:
 * 2800-3200-4800-5600-7800-10000 = 7200K (full range)
 * 0    400  2000 2800 5000 7200          (offset from 2800)
 * 0    14   71   100  178  255           (offset as 8 bit values for DMX) 
 * @param input The Kelvin value input 2800-10000(K)
 */
int convertKelvinToByte(int input) {
  // Limit inputs
  if(input < 2800) {return(0);}
  if(input > 10000) {return(255);}

  // Map ranges
  if(input < 3200) {
    return(round(map(input, 2800, 3200, 0, 14)));
  }
  if(input < 4800) {
    return(round(map(input, 3200, 4800, 14, 71)));
  }
  if(input < 5600) {
    return(round(map(input, 4800, 5600, 71, 100)));
  }
  if(input < 7800) {
    return(round(map(input, 5600, 7800, 100, 178)));
  }

  return(round(map(input, 7800, 10000, 178, 255)));
}

/**
 * Convert Byte To Kelvin
 *
 * Take 0-255 input and make it into 2800-10000 int:
 * 2800-3200-4800-5600-7800-10000 = 7200K (full range)
 * 0    400  2000 2800 5000 7200          (offset from 2800)
 * 0    14   71   100  178  255           (offset as 8 bit values for DMX) 
 * @param input The 8bit 0-255 value to be converted to Kelvin value
 */
int convertByteToKelvin(int input) {
  // Limit inputs
  if(input < 0) {return(2800);}
  if(input > 255) {return(10000);}

  // Map ranges
  if(input < 14) {
    return(round(map(input, 0, 14, 28, 32))*100);
  }
  if(input < 71) {
    return(round(map(input, 14, 71, 32, 48))*100);
  }
  if(input < 100) {
    return(round(map(input, 71, 100, 48, 56))*100);
  }
  if(input < 178) {
    return(round(map(input, 100, 178, 56, 78))*100);
  }

  return(round(map(input, 178, 255, 78, 100))*100);
}

int findMin(int inp1, int inp2, int inp3) {
  int localMin = inp1;
  if(inp2<localMin) localMin=inp2;
  if(inp3<localMin) localMin=inp3;
  return localMin;
}