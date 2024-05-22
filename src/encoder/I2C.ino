void sendColorToController() {
  // Send data to DMX controller
  Wire.beginTransmission(I2C_SLAVE_ADDRESS);
  for (int i = 0; i < 9; i++) {
    Wire.write((byte*)&currValue[i], 2); // Send each int16_t as two bytes
    sentData[i] = currValue[i];
  }
  uint8_t error = Wire.endTransmission();
  if (error) {
    Serial.printf("endTransmission: %u\n", error);
  }
  Wire.endTransmission();
}

void I2cComm(void * pvParameters) {
  while(true) {
    if(millis()>lastI2cUpdate+250 && millis()>lastI2cSent+500 && millis()>lastTurn+100) {
      requestColorFromSlave(true);
      delay(20);
      requestColorFromSlave(false);
      lastI2cUpdate = millis();
    }
  }
}


void requestColorFromSlave(bool early) {
  Wire.requestFrom(I2C_SLAVE_ADDRESS, 18); 
  int i = 0;
  while (Wire.available()) {
    int partA = Wire.read();
    int partB = Wire.read();
    receivedData[i] = partA + (partB * 256);
    i++;
  }

  if (early) return;

  // For some reason I seem to get 1 step too old info back, so I ignore the 1st difference
  if(sendAndRecieveDif()) {
    Serial.printf("Old values [%i:%i:%i:%i:%i:%i:%i:%i:%i]/[%i:%i:%i:%i:%i:%i:%i:%i:%i]\n", 
                                                      receivedData[0], 
                                                      receivedData[1], 
                                                      receivedData[2], 
                                                      receivedData[3],
                                                      receivedData[4],
                                                      receivedData[5],
                                                      receivedData[6],
                                                      receivedData[7],
                                                      receivedData[8],
                                                      sentData[0],
                                                      sentData[1],
                                                      sentData[2],
                                                      sentData[3],
                                                      sentData[4],
                                                      sentData[5],
                                                      sentData[6],
                                                      sentData[7],
                                                      sentData[8]); 
    // Update values
    for(int i = 0; i < 9; i++) {
      currValue[i] = receivedData[i];  
      }
  }
}

int findMin(int inp1, int inp2, int inp3) {
  int localMin = inp1;
  if(inp2<localMin) localMin=inp2;
  if(inp3<localMin) localMin=inp3;
  return localMin;
}

bool sendAndRecieveDif() {
  for(int i=0; i < 9; i++) {
    if(receivedData[i] != currValue[i]) {
      //Serial.printf("(%i)[%i:%i],", i, receivedData[i], currValue[i]);
      return true;
    }
  }
  return false;
}
