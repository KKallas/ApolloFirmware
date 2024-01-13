#include <Arduino.h>
//#include <esp_dmx.h> // 3.0.3 beta
#include <HardwareSerial.h>
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
int targetTempData  = 520;          // Temp we are aiming to keep the lamp in (200-640) val*0.125 = temp in Celsius 65C
int pwmValueFan  = 0;

                                    // UART
char UartReceivedChars[64];         // an array to store the received UART data
boolean UartNewData = false;        // if all characters until newline are recieved
const int storedLutSize = sizeof(int)*9*5*6;
const int storedDmxOffsetSize = sizeof(int);
const int storedRgbwSize = 5*sizeof(int);
const int storedRedLutSize = sizeof(int)*4*11;


                                    // DMX
int DmxOffset;                      // Offset from 512 addresses
HardwareSerial DMXSerial(1);
byte DmxData[6];      // DMX packet buffer
bool DmxIsConnected = false;        // Connected Flag
int oldDmx[] = {0,0,0,0,0};         // previous DMX packet RGBtF potion
int newDmx[] = {0,0,0,0,0};         // last DMX packet to make sure values have changed

                                    // Calibration                            
int current_calibration_A[4] = {0, 0, 0, 0};
int current_calibration_B[4] = {0, 0, 0, 0};
int current_calibration_mixed[4] = {0, 0, 0, 0};
int intesity_list[9]     = {0, 1, 36, 73, 109, 145, 181,  218,  255}; 
int intesity_list_log[9] = {0, 1, 32, 64, 128, 256, 512, 1024, 2048};
int wb_index_list[6] = {0,14,71,100,178,255}; 
int temperature_range[] = {-160, -80, 0, 80, 160, 240, 320, 400, 480, 560, 640};

int calibration_points[6][9][5];     // Calibartion points [Kelvin 0-5][8-1][Lux,R,G,B,W]
int color_calibration_points[4][11]; // Temp/Ouptput scale LUT [RGBW][lut point at temp sensor points {-160, -80, 0, 80, 160, 240, 320, 400, 480, 560, 640}]
bool disable_red_comp = false;
bool dmx_enable = true;
int UartbytesAvailable;


void UartComm(void * pvParameters) {
  for(;;) {
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

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
    delayMicroseconds(100);
  }
}

void ColorUpdate(void * pvParameters){
 
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
                    taskCore);      // Core where the task should run

                                // UART communications
  xTaskCreatePinnedToCore(
                    UartComm,       // Function to implement the task
                    "UartCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    1);             // Core where the task should run [0 empty core, 1 admin core]
  

  EEPROM.begin(storedLutSize+storedDmxOffsetSize+storedRgbwSize+storedRedLutSize);

  // Load Calibartion
  EEPROM.get(0, calibration_points);

  // Load DMX offset
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

  // 485(DMX) 
  DMXSerial.begin(250000, SERIAL_8N2, 26, -1);

  Serial.print("Welcome to Apollo lamp to use the terminal\n");
}

void tachoRead() {
  tachoCount += 1;
}

unsigned long lastBreak = millis();
unsigned long lastDmx;
bool newFrame = false;
bool oddFrame = true;
int array_index = 0;
char packet_content[513];

void loop() {
  int start_code; 
  int currByte = 0;
  int serialBufferCount = 0;
  
  if (dmx_enable) {
    serialBufferCount = DMXSerial.available();
    if (millis() - lastBreak > 100) { // This duration might need to be adjusted
      //newFrame = true;
      lastBreak = millis();
      //Serial.printf("buffer: %i\n", serialBufferCount);
      if (serialBufferCount>0) {
        if (array_index == 513) {
          Serial.printf("DMX packet time: %i\n", millis() - lastDmx);
          lastDmx = millis();
          Serial.printf("full dmx packet: %i %i %i %i %i\n", packet_content[0],packet_content[1],packet_content[2],packet_content[3], packet_content[4]);
          array_index = 0;
        }
        
        //DMXSerial.read(); // Read and ignore start code

        //start_code = DMXSerial.read();
        while(currByte < serialBufferCount) { // 500ms timeout
          // Dedect break condition and abandon frame
          if(array_index == 513) {
            break;
          }
          packet_content[array_index] = DMXSerial.read();
          if(packet_content[array_index]!=0) {
            Serial.printf("currByte[%i]: %i\n", array_index, packet_content[array_index]);
            }
          currByte++;
          array_index++;
        }
      } else {
        Serial.printf("break\n");
      }
      //DMXSerial.flush();
      
      //Serial.printf("currByte: %i\n", array_index);
  /*
      newDmx[0] = DmxData[0];
      newDmx[1] = DmxData[1];
      newDmx[2] = DmxData[2];
      newDmx[3] = DmxData[3];
      newDmx[4] = DmxData[4];

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
        targetTempData = 520;
      } else {
        targetTempData = map(Fan,1,256,30*8,80*8); // Temp range between 30-80C
      }

      if (powerSate && programSate) {
        set_dmx(newDmx[0], newDmx[1], newDmx[2], newDmx[3], Fan, true);
        pwmValueRed   = current_calibration_mixed[0];
        pwmValueGreen = current_calibration_mixed[1];
        pwmValueBlue  = current_calibration_mixed[2];
        pwmValueWhite = current_calibration_mixed[3];
        // direct passthrough
        //pwmValueRed    = DmxData[1 + DmxOffset];
        //pwmValueGreen  = DmxData[2 + DmxOffset];
        //pwmValueBlue   = DmxData[3 + DmxOffset];
        //pwmValueWhite  = DmxData[4 + DmxOffset];
        //
      }

      if(dmxDebugState) {
        Serial.printf("\"DMX\": [%i, %i, %i, %i, %i, %i], \"red_val\":%i, \"green_val\":%i, \"blue_val\":%i, \"white_val\":%i, \"TempTarget\":%i, \"offset\":%i, \"start_code\":%i, \"len\":%i\n",
                                                                            newDmx[0], newDmx[1], newDmx[2], newDmx[3], newDmx[4], DmxData[6],
                                                                            pwmValueRed, pwmValueGreen, pwmValueBlue, pwmValueWhite, 
                                                                            targetTempData, DmxOffset, start_code, index);
      }
    } else {
      delayMicroseconds(10000);
    }
    */
    } 
  }
}


bool arraysAreEqual(int arr1[], int arr2[]) {
  for (int i = 0; i < 5; i++) {
      if (arr1[i] != arr2[i]) {
          return false; // Arrays are not equal
      }
  }
  return true; // Arrays are equal
}