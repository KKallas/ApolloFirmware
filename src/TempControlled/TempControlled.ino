#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <limits.h>
#include <WiFi.h> // Only for MAC id
#include <EEPROM.h>
#include <ezButton.h> // 1.0.4

// IO pins
#define VERSION_4  // Define this macro for version 3 hardware, otherwise VERSION_4

#ifdef VERSION_3
const int redPin = 21;      // IO21 (Red Channel)
const int greenPin = 19;    // IO19 (Green Channel)
const int bluePin = 18;     // IO18 (Blue Channel)
const int whitePin = 4;     // IO04 (White Channel)
#else
const int redPin = 4;       // IO04 (Red Channel)
const int greenPin = 18;    // IO18 (Green Channel)
const int bluePin = 19;     // IO19 (Blue Channel)
const int whitePin = 21;    // IO21 (White Channel)
#endif
const int fanPin = 0;       // IO0  (Fan)          

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

const int pwmPin = 21;
unsigned long lastUpdate = millis();
unsigned long now = millis();
int tachoCount = 0;
int fanRpm = 0;
                                   // LIGHT
const int pwmFrequency   = 20000;  // 20 kHz
const int pwmResolution  = 11;     // 11-bit resolution (2048 levels)
const int blinkFrequency = 200;    // Update rate

int lastRGBW[4];

int pwmValueRed    = 0;           // Per channel data (do these need to be uint?)
int pwmValueGreen  = 0;
int pwmValueBlue   = 0;
int pwmValueWhite  = 0;

int compRedVal = 0;

                                    // TEMPERATURE
int currentTempData = 0;            // Temp sensor data RAW (multiply by 0.125 to get Celsius)
int TempArray[5] = {200, 200, 200, 200, 200}; // Initialize with default value 25*8 temp sensor RAW value
int nextTempArrayPos = 0;
int sortedArray[5]; 
bool overheated;
                                    // FAN
int FanSpeed = 0;
int FanSpeedStep = 32;
int targetTempData  = 520;          // Temp we are aiming to keep the lamp in (200-640) val*0.125 = temp in Celsius 65C
int pwmValueFan  = 0;

                                    // UART
char UartReceivedChars[512];        // an array to store the received UART data
boolean UartNewData = false;        // if all characters until newline are recieved
const int storedLutSize = sizeof(int)*9*5*6;
const int storedDmxOffsetSize = sizeof(int);
const int storedRgbwSize = 5*sizeof(int);
const int storedRedLutSize = sizeof(int)*4*11;


                                    // DMX
int DmxOffset;                      // Offset from 512 addresses TODO: replace or memory map

                                    // Calibration                            
int current_calibration_A[4] = {0, 0, 0, 0};
int current_calibration_B[4] = {0, 0, 0, 0};
int current_calibration_mixed[4] = {0, 0, 0, 0};
int intesity_list[9]     = {0, 1, 36, 73, 109, 145, 181,  218,  255}; 
int intesity_list_log[9] = {0, 1, 32, 64, 128, 256, 512, 1024, 2048};
int wb_index_list[6] = {0,14,71,100,178,255}; 
int temperature_range[] = {-160, -80, 0, 80, 160, 240, 320, 400, 480, 560, 640};

int calibration_points[6][9][5];     // Calibartion points [Kelvin 0-5][8-1][R, G, B, W, Lux]
int color_calibration_points[4][11]; // Temp/Ouptput scale LUT [RGBW][lut point at temp sensor points {-160, -80, 0, 80, 160, 240, 320, 400, 480, 560, 640}]
bool disable_red_comp = false;
int UartbytesAvailable;
long ButtonPressLog[2][16];
int ButtonIndex[2] = {0,0};

void setup() {
 
  Serial.begin(115200);
                                /* Enable All outputs */
  pinMode(EnabledPin, OUTPUT);              // Set IO27 as output
  digitalWrite(EnabledPin, HIGH);           // Enable output on IO27 (set it to HIGH)  

                                /* I2C */
  Wire.begin(I2cSdaPin, I2cSclPin);         // Wire needs always 2 pins


                                // Color Update 
                                    // 2nd core is used perly for the update and calibration
                                    // make sure not use any delays, otherwise it will drop out of sync 
  xTaskCreatePinnedToCore(
                    ColorUpdate,    // Function to implement the task
                    "ColorUpdate",  // Name of the task
                    10000,           // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    0);      // Core where the task should run

                                // UART communications
  xTaskCreatePinnedToCore(
                    UartComm,       // Function to implement the task
                    "UartCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    1);             // Core where the task should run [0 empty core, 1 admin core]
    xTaskCreatePinnedToCore(
                    TempUpdate,       // Function to implement the task
                    "TempUpdateHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    1);             // Core where the task should run [0 empty core, 1 admin core]
  

  EEPROM.begin(storedLutSize+storedDmxOffsetSize+storedRgbwSize+storedRedLutSize);

  // Load Calibartion
  EEPROM.get(0, calibration_points);

  // Load DMX offset TODO: reuse for something else or re-do the memory map
  EEPROM.get(storedLutSize,DmxOffset);

  // Load Colors
  int colors_in[4];
  EEPROM.get(storedLutSize+storedDmxOffsetSize,colors_in);
  pwmValueRed = colors_in[0];
  pwmValueGreen = colors_in[1];
  pwmValueBlue = colors_in[2];
  pwmValueWhite = colors_in[3];

  // Load Red calibartion curve
  EEPROM.get(storedLutSize+storedDmxOffsetSize+storedRgbwSize,color_calibration_points);

  buttonPower.setDebounceTime(50);
  buttonProgram.setDebounceTime(50);

  // Fan Tacho
  attachInterrupt(TachoPin, tachoRead, FALLING);

  Serial.print("Welcome to Apollo lamp to use the terminal\n");
}

void loop() {
  sleep(1);
  
}