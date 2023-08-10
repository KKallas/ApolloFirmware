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

int pwmValue1 = 0;
int pwmValue2 = 0;
int currentTempData = 0;

char receivedChars[32];             // an array to store the received UART data
boolean newData = false;            // if all characters until newline are recieved

void ColorUpdate( void * pvParameters ){
 
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

    ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(1, pwmFrequency, pwmResolution); // Configure PWM
    ledcAttachPin(pwmPin, 0);       // Attach PWM to IO22 (Red Channel)
    ledcAttachPin(0, 1);            // Fan

    while(true){                    // Alternately set the PWM duty cycle to pwmValue1 and pwmValue2, Half-period delay
      ledcWrite(0, pwmValue1*8);
      ledcWrite(1, pwmValue2*8);
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}
 
void setup() {
 
  Serial.begin(115200);
 
  Serial.print("Creating services ");
  Serial.println(taskCore);

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
 
  Serial.println("Task created...");
  Serial.println("Starting main loop...");
}
 
void loop() {
  int temperatureData;
  int bytesAvailable;

  if (now - lastUpdate > 1000) {
    Wire.requestFrom(78,2);
    while(Wire.available()) {
      temperatureData = (Wire.read() << 8) | Wire.read();
      temperatureData = temperatureData >> 5;
      currentTempData = temperatureData;
    }

    /* Print the received start code - it's usually 0. */
    //Serial.printf("time:%i,dmx_fan:%02X,dmx_val:%02X,temp:%f\n", lastUpdate, data[511], data[512],temperatureData*0.125);
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
      if (ndx >= 32) {
        ndx = 32 - 1;
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
    char IntMS;
    char IntLS;
    int Intensity;
    int Fan;

    if (newData == true && receivedChars[0] == 'A') {
      IntMS = receivedChars[1];
      IntLS = receivedChars[2];
      Fan = receivedChars[3];

      Intensity = ((int)IntMS << 8) | (int)(IntLS & 0xFF);

      Serial.printf("dmx_val:%02X,dmx_fan:%02X,temp:%f\n", Intensity, Fan, currentTempData*0.125);
      newData = false;
  }
}