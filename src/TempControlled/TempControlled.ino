#include <Arduino.h>
#include <esp_dmx.h> // 3.0.3 beta
#include <Wire.h>
#include <limits.h>
#include <WiFi.h> // Only for MAC id
#include <EEPROM.h>
#include <ezButton.h> // 1.0.4

// IO pins
const int redPin = 21;            // IO21 (Red Channel)
const int greenPin = 19;          // IO19 (Green Channel) 
const int bluePin = 18;           // IO18 (Blue Channel)
const int whitePin = 4;           // IO04 (White Channel)
const int fanPin = 0;             // IO0  (Fan)          

const int EnabledPin = 27;
const int I2cSdaPin = 23;
const int I2cSclPin = 22;
const int TachoPin = 25;
const int DmxReceivePin = 26;      // ESP32 pin (SK led Pin)
const int tachFanPulses = 2;

ezButton buttonPower(34);
ezButton buttonProgram(35);

bool powerSate = true;
bool programSate = true;
bool dmxDebugState = false;

const int taskCore = 0;
const int pwmPin = 21;
unsigned long lastUpdate = millis();
unsigned long now = millis();
int tachoCount = 0;
int fanRpm = 0;
                                   // LIGHT
const int pwmFrequency   = 20000;  // 20 kHz
const int pwmResolution  = 11;     // 11-bit resolution (2048 levels)
const int blinkFrequency = 200;    // Update rate
const int STEPS          = 50;     // Interpolation setps 1/200*steps = interpolation time in seconds

int lastRGBW[4];

int pwmValueRed    = 0;           // Per channel data (do these need to be uint?)
int pwmValueGreen  = 0;
int pwmValueBlue   = 0;
int pwmValueWhite  = 0;

int compRedVal = 0;

int targetValueRed   = 0;          // Point chaser variable (currently using just fan)
int targetValueGreen = 0;
int targetValueBlue  = 0;
int targetValueWhite = 0;

int stepRed   = 0;
int stepGreen = 0;
int stepBlue  = 0;
int stepWhite = 0;

                                    // TEMPERATURE
int currentTempData = 0;            // Temp sensor data RAW (multiply by 0.125 to get Celsius)
int TempArray[5] = {200, 200, 200, 200, 200}; // Initialize with default value 25*8 temp sensor RAW value
int nextTempArrayPos = 0;
int sortedArray[5]; 
bool overheated;
                                    // FAN
int FanSpeed = 0;
int FanSpeedStep = 128;
int targetTempData  = 520;          // Temp we are aiming to keep the lamp in (200-640) val*0.125 = temp in Celsius
int pwmValueFan  = 0;

                                    // UART
char UartReceivedChars[64];         // an array to store the received UART data
boolean UartNewData = false;        // if all characters until newline are recieved
const int storedLutSize = sizeof(int)*9*5*6;
const int storedArtnetOffsetSize = 16;
const int storedRgbwSize = 4*16;

                                    // DMX
int DmxOffset;                      // Offset from 512 addresses
dmx_port_t DmxPort = 1;             // Built in serial port HW
byte DmxData[DMX_PACKET_SIZE];      // DMX packet buffer
bool DmxIsConnected = false;        // Connected Flag
int oldDmx[] = {0,0,0,0,0};
int newDmx[] = {0,0,0,0,0};

                                    // Calibration                            
int current_calibration_A[4] = {0, 0, 0, 0};
int current_calibration_B[4] = {0, 0, 0, 0};
int current_calibration_mixed[4] = {0, 0, 0, 0};
int intesity_list[9]     = {0, 1, 36, 73, 109, 145, 181,  218,  255}; 
int intesity_list_log[9] = {0, 1, 32, 64, 128, 256, 512, 1024, 2048};
int wb_index_list[6] = {0,14,71,100,178,255}; 

int calibration_points[6][9][4];
bool disable_red_comp = false;

void ColorUpdate( void * pvParameters ){
 
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

    ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
    ledcSetup(1, pwmFrequency, pwmResolution); 
    ledcSetup(2, pwmFrequency, pwmResolution); 
    ledcSetup(3, pwmFrequency, pwmResolution); 
    ledcSetup(4, pwmFrequency, pwmResolution); 
    ledcAttachPin(redPin, 0);
    ledcAttachPin(greenPin, 1);
    ledcAttachPin(bluePin, 2);
    ledcAttachPin(whitePin, 3);
    ledcAttachPin(fanPin, 4);

    while(true){
      buttonPower.loop();
      buttonProgram.loop();
      
      if (buttonPower.isPressed()) {
        if (powerSate) {
          powerSate = false;
        } else {
          powerSate = true;
        }
      }

      if(buttonProgram.isPressed()) {
        if (programSate) {
          programSate = false;
          lastRGBW[0] = pwmValueRed;
          lastRGBW[1] = pwmValueGreen;
          lastRGBW[2] = pwmValueBlue;
          lastRGBW[3] = pwmValueWhite;
          set_dmx(255,255,255,100,0,false);
          pwmValueRed = current_calibration_mixed[0];
          pwmValueGreen = current_calibration_mixed[1];
          pwmValueBlue = current_calibration_mixed[2];
          pwmValueWhite = current_calibration_mixed[3];
        } else {
          programSate = true;
          pwmValueRed = lastRGBW[0];
          pwmValueGreen = lastRGBW[1];
          pwmValueBlue = lastRGBW[2];
          pwmValueWhite = lastRGBW[3];
        }
      }
      if (overheated == false) {    // Overheting protection

                                    // Red comp with disable option, all other than redcalib should set disable_red_comp = false;
        if (disable_red_comp==true) {
          compRedVal = pwmValueRed;
        } else {
          compRedVal = calculateRedColor(pwmValueRed, currentTempData);
        }

                                    // Point Chaser interpolations
        // pwmValueFan = updateChannel(pwmValueFan,targetValueFan,stepFan);

                                    // Set Color
        /*
        The difference with overheatring is we want the normal Fan functionality to continue even after blackout
        Therefore toggle of RGBW channels and fan is handeled sepparately later
        */
        if (powerSate) {
          ledcWrite(0, compRedVal);
          ledcWrite(1, pwmValueGreen);
          ledcWrite(2, pwmValueBlue);
          ledcWrite(3, pwmValueWhite);
        } else {
          ledcWrite(0, 0);
          ledcWrite(1, 0);
          ledcWrite(2, 0);
          ledcWrite(3, 0);
        }
        ledcWrite(4, pwmValueFan);
      } else {
        ledcWrite(0, 0);
        ledcWrite(1, 0);
        ledcWrite(2, 0);
        ledcWrite(3, 0);
        ledcWrite(4, 2047);
      }
      // Wait for next update
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}
 
void setup() {
 
  Serial.begin(115200);
                                /* Enable All outputs */
  pinMode(EnabledPin, OUTPUT);              // Set IO27 as output
  digitalWrite(EnabledPin, HIGH);           // Enable output on IO27 (set it to HIGH)  

                                /* I2C */
  Wire.begin(I2cSdaPin, I2cSclPin);         // Wire needs always 2 pins

                                /* 485(DMX) */

  dmx_driver_install(DmxPort, DMX_DEFAULT_INTR_FLAGS);
  dmx_set_pin(DmxPort, 0, DmxReceivePin, 0);

                                // Color Update 
                                    // 2nd core is used perly for the update and calibration
                                    // make sure not use any delays, otherwise it will drop out of sync 
  xTaskCreatePinnedToCore(
                    ColorUpdate,    // Function to implement the task
                    "ColorUpdate",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    taskCore);      // Core where the task should run

  EEPROM.begin(storedLutSize+storedArtnetOffsetSize+storedRgbwSize);

  // Load Calibartion
  EEPROM.get(0, calibration_points);

  // Load DMX offset
  EEPROM.get(storedLutSize+1,DmxOffset);

  // Load Colors
  EEPROM.get(storedLutSize+storedArtnetOffsetSize+1,pwmValueRed);
  EEPROM.get(storedLutSize+storedArtnetOffsetSize+16+1,pwmValueGreen);
  EEPROM.get(storedLutSize+storedArtnetOffsetSize+32+1,pwmValueBlue);
  EEPROM.get(storedLutSize+storedArtnetOffsetSize+48+1,pwmValueWhite);

  buttonPower.setDebounceTime(50);
  buttonProgram.setDebounceTime(50);

  // Fan Tacho
  attachInterrupt(TachoPin, tachoRead, FALLING);

  Serial.print("Welcome to Apollo lamp to use the terminal\n");
}

void tachoRead() {
  tachoCount += 1;
}

void loop() {
  dmx_packet_t packet;

  if (dmx_receive(DmxPort, &packet, DMX_TIMEOUT_TICK)) {
    if (!packet.err) {
      if (!DmxIsConnected) {
        DmxIsConnected = true;
      }

      // Read DMX and set the color values
      dmx_read(DmxPort, DmxData, packet.size);

      newDmx[0] = DmxData[1 + DmxOffset];
      newDmx[1] = DmxData[2 + DmxOffset];
      newDmx[2] = DmxData[3 + DmxOffset];
      newDmx[3] = DmxData[4 + DmxOffset];
      newDmx[4] = DmxData[5 + DmxOffset];

      // If it is the same packet nothing fruther to do
      if (arraysAreEqual(newDmx,oldDmx) == true) {
        return;
      }

      // Copy to old so it would be possible to check
      for (int i = 0; i < 5; i++) {
        oldDmx[i] = newDmx[i];
      }

      // If no Fan value is provided then (codevalue 0) then use 65C
      int Fan = newDmx[4];
      if (Fan == 0) {
        Fan = 520;
      } else {
        targetTempData = map(Fan,1,256,30*8,80*8); // Temp range between 30-80C
      }

      if(dmxDebugState) {
        Serial.printf("DMX: %i %i %i %i %i\n",newDmx[0], newDmx[1], newDmx[2], newDmx[3], newDmx[4]);
      }

      if (powerSate && programSate) {
        set_dmx(newDmx[0], newDmx[1], newDmx[2], newDmx[3], Fan, true);
        
        pwmValueRed   = current_calibration_mixed[0];
        pwmValueGreen = current_calibration_mixed[1];
        pwmValueBlue  = current_calibration_mixed[2];
        pwmValueWhite = current_calibration_mixed[3];
        /* direct passthrough
        pwmValueRed    = DmxData[1 + DmxOffset];
        pwmValueGreen  = DmxData[2 + DmxOffset];
        pwmValueBlue   = DmxData[3 + DmxOffset];
        pwmValueWhite  = DmxData[4 + DmxOffset];
        */
      }
    } 
  }

  int bytesAvailable;
  now = millis();

  if (now - lastUpdate > 5000) {
    fanRpm = tachoCount * 60 / 5 / tachFanPulses; 
    tachoCount = 0;
    readTemp();
    updateFanSpeed();           
    //Serial.printf("time:%i,dmx_fan:%02X,dmx_val:%02X,temp:%f\n", lastUpdate, data[511], data[512],temperatureData*0.125);
    //Serial.printf("time:%i\n", lastUpdate);
    //Serial.printf("TempArray:%i,%i,%i,%i,%i, valueSelected:%i\n", sortedArray[0], sortedArray[1], sortedArray[2], sortedArray[3], sortedArray[4], currentTempData);

    lastUpdate = now;
  }
  // Add loop to handle UART input and output
  RecvUart();
  HandleUartCmd();
  usleep(10);
}


bool arraysAreEqual(int arr1[], int arr2[]) {
  for (int i = 0; i < 5; i++) {
      if (arr1[i] != arr2[i]) {
          return false; // Arrays are not equal
      }
  }
  return true; // Arrays are equal
}