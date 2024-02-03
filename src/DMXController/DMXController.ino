#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <limits.h>
#include <WiFi.h> // Only for MAC id
#include <esp_dmx.h>
#include <EEPROM.h>
       
const int I2cSdaPin = 25;
const int I2cSclPin = 21;
const int DmxRxPin = 32; 
const int DmxTxPin = 26; 
const int Serial2Tx = 19;
const int Serial2Rx = 22;

dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;

unsigned long lastUpdate = millis();
unsigned long now = millis();
unsigned long processTimer1[2];

char Uart1ReceivedChars[256];        // an array to store the received UART data
char Uart2ReceivedChars[256];        
boolean UartNewData = false;        // if all characters until newline are recieved

int packetsPerSec;
int lastVal[4] = {0,0,0,0};

void setup() {
 
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1, Serial2Rx, Serial2Tx);

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_driver_install(dmxPort, &config, DMX_INTR_FLAGS_DEFAULT);
  dmx_set_pin(dmxPort, DmxTxPin, DmxRxPin, 0);

  Wire.begin(I2cSdaPin, I2cSclPin);         // Wire needs always 2 pins


                                // UART communications
  xTaskCreatePinnedToCore(
                    UartComm,       // Function to implement the task
                    "UartCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    1);             // Core where the task should run [0 empty core, 1 admin core]

  Serial.print("Welcome to Apollo lamp to use the terminal\n");
}

void loop() {
  dmx_packet_t packet;
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    bool NewValue = false;
    unsigned long Serial2PrintTime = now;
    if (!packet.err) {
      if (!dmxIsConnected) {
        dmxIsConnected = true;
      }

      Serial2PrintTime = millis();
      dmx_read(dmxPort, data, packet.size);
      packetsPerSec = packetsPerSec + 1;

      for(int iloc = 0;iloc < 4;iloc++) {
        if (data[iloc+1] != lastVal[iloc]) {
          lastVal[iloc] = data[iloc+1];
          NewValue = true;
        }
      }
    
      if (NewValue) {
        SendUartCmd("DS02->"+String(data[1])+":"+String(data[2])+":"+String(data[3])+":"+String(data[4]), true, true);
      }

      Serial2PrintTime = millis() - Serial2PrintTime;
      
      if (now - lastUpdate > 1000) {
        /* Print the received start code - it's usually 0. */
        Serial.printf("DS10->last DMX packet [%i:%i:%i:%i]//0000:::\n", data[1], data[2], data[3], data[4]);
        packetsPerSec = 0;
        lastUpdate = now;
      }
    } 
  }
  
}