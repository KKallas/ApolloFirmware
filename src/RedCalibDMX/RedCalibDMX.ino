#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>

const uint taskCore = 0;
const uint pwmPin = 21;
unsigned long lastUpdate = millis();
unsigned long now = millis();
                                     // LIGHT
const uint pwmFrequency = 20000;     // 20 kHz
const uint pwmResolution = 11;       // 11-bit resolution (2048 levels)
const uint blinkFrequency = 200;     // Update rate
uint pwmValueRed = 0;                // Per channel data
uint pwmValueGreen = 0;
uint pwmValueBlue = 0;
uint pwmValueWhite = 0;
uint pwmValueFan = 0;
uint currentTempData = 0;
                                    // UART
char UartReceivedChars[64];         // an array to store the received UART data
boolean UartNewData = false;        // if all characters until newline are recieved

                                    // DMX
const uint DmxReceivePin = 26;      // ESP32 pin (SK led Pin)
uint DmxOffset = 0;                 // Offset from 512 addresses
dmx_port_t DmxPort = 1;             // Built in serial port HW
byte DmxData[DMX_PACKET_SIZE];      // DMX packet buffer
bool DmxIsConnected = false;        // Connected Flag

void ColorUpdate( void * pvParameters ){
 
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

    ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(1, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(2, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(3, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(4, pwmFrequency, pwmResolution); // Configure PWM
    ledcAttachPin(21, 0);           // Attach PWM to IO21 (Red Channel)
    ledcAttachPin(19, 1);           // Attach PWM to IO19 (Green Channel) 
    ledcAttachPin(18, 2);           // Attach PWM to IO18 (Blue Channel)
    ledcAttachPin(4, 3);            // Attach PWM to IO04 (White Channel) 
    ledcAttachPin(0, 4);            // Fan IO0

    while(true){
      int compRedVal = calculateRedColor(pwmValueRed, currentTempData>>3);
      ledcWrite(0, compRedVal);
      ledcWrite(1, pwmValueGreen);
      ledcWrite(2, pwmValueBlue);
      ledcWrite(3, pwmValueWhite);
      ledcWrite(4, pwmValueFan*8);
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}

int calculateRedColor(uint redColor, uint temperature) {
    // Calculation on 16bit out 11bit
    // 85C on max = 2047
    // @30 on 0.66 = 1351
    // @-25 on 0.33 = 676
    uint tempOffset = 85-temperature;
    uint tempCompen = 65535-(tempOffset*395);

    // lookat map

    // Adjust the red color value based on the provided redColor parameter
    uint redColorValue = (tempCompen * (redColor << 5)); // 16bit theorticalMax*actualValue
    redColorValue = redColorValue >> 16;                 // devide by 65K to normalize the answer
    redColorValue = redColorValue >> 5;                  // downscale to 11bits
    return (uint)redColorValue;
}
 
void setup() {
 
  Serial.begin(115200);

  pinMode(27, OUTPUT);              // Set IO27 as output
  digitalWrite(27, HIGH);           // Enable output on IO27 (set it to HIGH)  
 
  Wire.begin(23,22);                // Wire needs always 2 pins

  dmx_set_pin(DmxPort, 0, DmxReceivePin, 0);
  dmx_driver_install(DmxPort, DMX_DEFAULT_INTR_FLAGS);

  xTaskCreatePinnedToCore(
                    ColorUpdate,    // Function to implement the task
                    "ColorUpdate",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    taskCore);      // Core where the task should run
}
 
void loop() {
  // TODO: temperature buffering
  dmx_packet_t packet;

  if (dmx_receive(DmxPort, &packet, DMX_TIMEOUT_TICK)) {
    if (!packet.err) {
      if (!DmxIsConnected) {
        DmxIsConnected = true;
      }

      // Read DMX and set the color values
      dmx_read(DmxPort, DmxData, packet.size);
      pwmValueRed   = DmxData[1 + DmxOffset]*8;
      pwmValueGreen = DmxData[2 + DmxOffset]*8;
      pwmValueBlue  = DmxData[3 + DmxOffset]*8;
      pwmValueWhite = DmxData[4 + DmxOffset]*8;
      // DMX fan mode if 0 the 1024 else the actual value
      int DmxFanValue = DmxData[5 + DmxOffset];
      if (DmxFanValue == 0) {
        pwmValueFan = 2048;
      }
      else {
        pwmValueFan = DmxFanValue*8;
      }
      Serial.print(DmxFanValue);
    } 
  }


  int temperatureData;
  int bytesAvailable;
  now = millis();

  if (now - lastUpdate > 1000) {
    Wire.requestFrom(78,2);
    while(Wire.available()) {
      temperatureData = (Wire.read() << 8) | Wire.read();
      temperatureData = temperatureData >> 5;
      currentTempData = temperatureData;

      //Serial.print("ping\n");
    }

    //Serial.printf("time:%i,dmx_fan:%02X,dmx_val:%02X,temp:%f\n", lastUpdate, data[511], data[512],temperatureData*0.125);
    //Serial.printf("time:%i\n", lastUpdate);
    lastUpdate = now;
    
  }
  // Add loop to handle UART input and output
  RecvUart();
  HandleUartCmd();
  
  sleep(0.001);
}

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
    uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite;
    uint Fan;

    if (UartNewData == true && UartReceivedChars[0] == 'A') {
      // Red Channel
      IntensityRed = twoByteChar(UartReceivedChars[1],UartReceivedChars[2]);
      // Green Channel
      IntensityGreen = twoByteChar(UartReceivedChars[3],UartReceivedChars[4]);
      // Blue Channel
      IntensityBlue = twoByteChar(UartReceivedChars[5],UartReceivedChars[6]);
      // White Channel
      IntensityWhite = twoByteChar(UartReceivedChars[7],UartReceivedChars[8]);
      // Fan
      Fan = UartReceivedChars[9];

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan = Fan;
      
      UartNewData = false;
    }
    if (UartNewData == true && UartReceivedChars[0] == 'D') {
      // Red Channel
      IntensityRed = twoByteChar(0,2);
      // Green Channel
      IntensityGreen = twoByteChar(0,0);
      // Blue Channel
      IntensityBlue = twoByteChar(0,0);
      // White Channel
      IntensityWhite = twoByteChar(0,0);
      // Fan
      Fan = 0x88;

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan = Fan;
      
      UartNewData = false;
    }
  }


int twoByteChar(char byte1,char byte2) {
  uint output = ((uint)byte1 << 8) | (uint)(byte2 & 0xFF);
  return(output);
}