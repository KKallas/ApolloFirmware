#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>

const uint taskCore = 0;
const uint pwmPin = 21;
const uint pwmFrequency = 20000;     // 20 kHz
const uint pwmResolution = 11;       // 11-bit resolution (2048 levels)
const uint blinkFrequency = 200;

unsigned long lastUpdate = millis();
unsigned long now = millis();

uint pwmValueRed = 0;
uint pwmValueGreen = 0;
uint pwmValueBlue = 0;
uint pwmValueWhite = 0;
uint pwmValueFan = 0;
uint currentTempData = 0;

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
 
  while (Serial.available() > 0 && newData == false) {
    char lastChar;
    rc = Serial.read();

    if ((lastChar != '*' ) && (rc != endMarker)) {
      receivedChars[ndx] = rc;
      lastChar = rc;
      ndx++;
      if (ndx >= 64) {
        ndx = 64 - 1;
      }
      Serial.print(rc);
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void HandleUartCmd() {
    uint IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite;
    uint Fan;

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

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);

      pwmValueRed = IntensityRed;
      pwmValueGreen = IntensityGreen;
      pwmValueBlue = IntensityBlue;
      pwmValueWhite = IntensityWhite;
      pwmValueFan = Fan;
      
      newData = false;
    }
    if (newData == true && receivedChars[0] == 'D') {
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
      
      newData = false;
    }
  }


int twoByteChar(char byte1,char byte2) {
  uint output = ((uint)byte1 << 8) | (uint)(byte2 & 0xFF);
  return(output);
}