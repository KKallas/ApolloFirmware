#include <Wire.h>
#include <limits.h>
#include <WiFi.h> // Only for MAC id
#include <esp_dmx.h>
#include <EEPROM.h>

#define I2C_SLAVE_ADDRESS 8

const int I2cSdaPin = 25;
const int I2cSclPin = 21;
const int DmxRxPin = 32; 
const int DmxTxPin = 26; 
const int Serial2Tx = 19;
const int Serial2Rx = 22;

dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
bool dmxEnabled = true;

int16_t toSendData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Data to send I2C & Recieve buffer
int recievedData[9];
int dmxOffset=0;

unsigned long lastUpdate = millis();
unsigned long now = millis();
unsigned long processTimer1[2];

char Uart1ReceivedChars[256];        // an array to store the received UART data
char Uart2ReceivedChars[256];        
boolean UartNewData = false;        // if all characters until newline are recieved

int packetsPerSec;
int lastValDMX[4] = {0,0,0,0};
int lastValI2C[9] = {0,0,0,0,0,0,0,0,0}; // R,G,B,kelvin temp,dmx offset,dmx enabled

void setup() {
 
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1, Serial2Rx, Serial2Tx);

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_driver_install(dmxPort, &config, DMX_INTR_FLAGS_DEFAULT);
  dmx_set_pin(dmxPort, DmxTxPin, DmxRxPin, 0);

  Wire.begin(I2C_SLAVE_ADDRESS, I2cSdaPin, I2cSclPin, 100000);        // Wire needs always 2 pins
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
                                // UART communications
  xTaskCreatePinnedToCore(
                    UartComm,       // Function to implement the task
                    "UartCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    0);             // Core where the task should run [0 empty core, 1 admin core]
  
  /*xTaskCreatePinnedToCore(
                    I2cComm,       // Function to implement the task
                    "I2cCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    1);             // Core where the task should run [0 empty core, 1 admin core]
*/
  Serial.print("Welcome to Apollo lamp to use the terminal\n");
}

void loop() {
  dmx_packet_t packet;
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    bool NewValue = false;
    unsigned long Serial2PrintTime = now;
    if (!packet.err && dmxEnabled) {
      if (!dmxIsConnected) {
        dmxIsConnected = true;
      }

      Serial2PrintTime = millis();
      dmx_read(dmxPort, data, packet.size);
      packetsPerSec = packetsPerSec + 1;

      for(int iloc = dmxOffset; iloc < 4 + dmxOffset; iloc++) {
        if (data[iloc+1] != lastValDMX[iloc - dmxOffset]) {
          lastValDMX[iloc - dmxOffset] = data[iloc+1];
          NewValue = true;
        }
      }
    
      if (NewValue) {
        SendUartCmd("DS02->"+String(data[1 + dmxOffset])+":"+String(data[2 + dmxOffset])+":"+String(data[3 + dmxOffset])+":"+String(data[4 + dmxOffset]), true, true);
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