void receiveEvent(int howMany) {
  int i = 0;
  while (Wire.available()) {
    int partA = Wire.read();
    int partB = Wire.read();
    recievedData[i] = partA + (partB * 256);
    i++;
  }
  SendUartCmd("DS02->"+String(recievedData[2])+":"+String(recievedData[3])+":"+String(recievedData[4])+":"+String(convertKelvinToByte(recievedData[1])), true, true);
  // Get DMX enabled fom GUI
  dmxEnabled = recievedData[6];
  lastValI2C[6] = dmxEnabled;
}

void requestEvent() {
  for(int i = 0; i < 9; i++) {
    Wire.write((byte *)&lastValI2C[i],2); // Send the current color back to the master
  }
}

int convertKelvinToByte(int input) {
  int value = (input-2800)/28;
  if(value > 255) value = 255;
  return value;
}

int convertByteToKelvin(int input) {
  int value = (input*28)+2800;
  return value;
}

int findMin(int inp1, int inp2, int inp3) {
  int localMin = inp1;
  if(inp2<localMin) localMin=inp2;
  if(inp3<localMin) localMin=inp3;
  return localMin;
}