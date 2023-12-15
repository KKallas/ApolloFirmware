/*
 [ ] Add sACN
 [ ] AP/Client (Atom RGB led)
 [ ] Create homepage to show my ip
 [ ] Webpage sliders
*/
#include <Arduino.h>
#include <esp_dmx.h>

int transmitPin = 26;
int receivePin = 32;
int enablePin = 0;
dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
unsigned long lastUpdate = millis();

                                    // UART
char UartReceivedChars[64];         // an array to store the received UART data
boolean UartNewData = false;        // if all characters until newline are recieved

int IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, TempTarget, offset;

void setup() {
  Serial.begin(115200);
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
  dmx_driver_install(dmxPort, DMX_DEFAULT_INTR_FLAGS);
}

void loop() {
  unsigned long now = millis();
  
  RecvUart();
  HandleUartCmd();

  if (now - lastUpdate >= 33) {
    memset(data, 0, sizeof(data));
    data[1+offset] = byte(IntensityRed);
    data[2+offset] = byte(IntensityGreen);
    data[3+offset] = byte(IntensityBlue);
    data[4+offset] = byte(IntensityWhite);
    data[5+offset] = byte(TempTarget);
    dmx_write(dmxPort, data, sizeof(data));
    lastUpdate = now;
  }

  dmx_send(dmxPort, DMX_PACKET_SIZE);
  dmx_wait_sent(dmxPort, DMX_TIMEOUT_TICK);
}

void RecvUart() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
 
  while (Serial.available() > 0 && UartNewData == false) {
    char lastChar;
    rc = Serial.read();

    if (rc != endMarker) {
      UartReceivedChars[ndx] = rc;
      lastChar = rc;
      ndx++;
      if (ndx >= 64) {
        ndx = 64 - 1;
      }
    }
    else {
      UartReceivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      UartNewData = true;
    }
  }
}

void HandleUartCmd() {
  if (UartNewData == true && UartReceivedChars[0] == 'D') {
    sscanf(UartReceivedChars, "D%i %i %i %i %i %i", &IntensityRed, &IntensityGreen, &IntensityBlue, &IntensityWhite, &TempTarget, &offset);
    Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"temp_target\":%i,\"offset\":%i\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, TempTarget, offset);
    UartNewData = false;
  }
  if (UartNewData == true && UartReceivedChars[0] == 'I') {
    Serial.printf("debug red: %i\n", IntensityRed);
    Serial.printf("debug data: %i\n", data[0]);
  }
  if (UartNewData == true) {
    Serial.print("Unkown command...\n");
    UartNewData = false;
  }
}
