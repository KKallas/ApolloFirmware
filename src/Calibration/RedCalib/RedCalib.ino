#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>

const int taskCore = 0;
const int pwmPin = 21;
const int pwmFrequency = 20000;     // 20 kHz
const int pwmResolution = 11;       // 11-bit resolution (2048 levels)
const int blinkFrequency = 200;

unsigned long lastUpdate = millis();
unsigned long now = millis();

int pwmValueRed = 0;
int pwmValueGreen = 0;
int pwmValueBlue = 0;
int pwmValueWhite = 0;
int pwmValueFan = 0;
int currentTempData = 0;

char receivedChars[64];             // an array to store the received UART data
boolean newData = false;            // if all characters until newline are recieved

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
    uint redColorValue = (tempCompen * (redColor << 5));
    redColorValue = redColorValue >> 16;
    redColorValue = redColorValue >> 5;
    //Serial.print(redColorValue,DEC);
    //Serial.print("\n");
    return (uint)redColorValue;
}
 
void setup() {
 
  Serial.begin(115200);

  pinMode(27, OUTPUT);              // Set IO27 as output
  digitalWrite(27, HIGH);           // Enable output on IO27 (set it to HIGH)  
 
  Wire.begin(23,22);                // As we aure using default pins 21 & 22

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
  int temperatureData;
  int bytesAvailable;
  now = millis();

  if (now - lastUpdate > 1000) {
    Wire.requestFrom(78,2);
    while(Wire.available()) {
      temperatureData = (Wire.read() << 8) | Wire.read();
      temperatureData = temperatureData >> 5;
      currentTempData = temperatureData;
    }

    //Serial.printf("time:%i,dmx_fan:%02X,dmx_val:%02X,temp:%f\n", lastUpdate, data[511], data[512],temperatureData*0.125);
    //Serial.printf("time:%i\n", lastUpdate);
    lastUpdate = now;
    
  }
  // Add loop to handle UART input and output
  RecvUart();
  HandleUartCmd();
}

void RecvUart() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
 
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= 64) {
        ndx = 64 - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void HandleUartCmd() {
    int IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite;
    int Fan;

    if (newData == true && receivedChars[0] == 'A') {
      // Red Channel
      IntensityRed = twoByteChar(receivedChars[1],receivedChars[2]);
      // Green Channel
      IntensityGreen = twoByteChar(receivedChars[3],receivedChars[4]);
      // Blue Channel
      IntensityBlue = twoByteChar(receivedChars[5],receivedChars[6]);
      // White Channel
      IntensityWhite = twoByteChar(receivedChars[7],receivedChars[8]);
      // Fan
      Fan = receivedChars[9];

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan = Fan;
      
      newData = false;
  }
}

int twoByteChar(char byte1,char byte2) {
  int output = ((int)byte1 << 8) | (int)(byte2 & 0xFF);
  return(output);
}