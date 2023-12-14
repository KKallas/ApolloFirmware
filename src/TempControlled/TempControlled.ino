#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>
#include <limits.h>
#include <WiFi.h> // Only for MAC id
#include <EEPROM.h>

const int taskCore = 0;
const int pwmPin = 21;
unsigned long lastUpdate = millis();
unsigned long now = millis();
                                   // LIGHT
const int pwmFrequency   = 20000;  // 20 kHz
const int pwmResolution  = 11;     // 11-bit resolution (2048 levels)
const int blinkFrequency = 200;    // Update rate
const int STEPS          = 50;     // Interpolation setps 1/200*steps = interpolation time in seconds


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
const int storedLutSize = sizeof(int)*9*4*6;
const int storedArtnetOffsetSize = 16;
const int storedRgbwSize = 4*16;

                                    // DMX
const int DmxReceivePin = 26;       // ESP32 pin (SK led Pin)
int DmxOffset;                      // Offset from 512 addresses
dmx_port_t DmxPort = 1;             // Built in serial port HW
byte DmxData[DMX_PACKET_SIZE];      // DMX packet buffer
bool DmxIsConnected = false;        // Connected Flag

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
    ledcAttachPin(21, 0);           // Attach PWM to IO21 (Red Channel)
    ledcAttachPin(19, 1);           // Attach PWM to IO19 (Green Channel) 
    ledcAttachPin(18, 2);           // Attach PWM to IO18 (Blue Channel)
    ledcAttachPin(4, 3);            // Attach PWM to IO04 (White Channel) 
    ledcAttachPin(0, 4);            // Fan IO0

    while(true){
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
        ledcWrite(0, compRedVal);
        ledcWrite(1, pwmValueGreen);
        ledcWrite(2, pwmValueBlue);
        ledcWrite(3, pwmValueWhite);
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
  pinMode(27, OUTPUT);              // Set IO27 as output
  digitalWrite(27, HIGH);           // Enable output on IO27 (set it to HIGH)  

                                /* I2C */
  Wire.begin(23,22);                // Wire needs always 2 pins

                                /* 485(DMX/RDM) */
  dmx_set_pin(DmxPort, 0, DmxReceivePin, 0);
  dmx_driver_install(DmxPort, DMX_DEFAULT_INTR_FLAGS);

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

  Serial.print("Welcome to Apollo lamp to use the terminal\n");
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

      // If no Fan value is provided then (codevalue 0) then use 65C
      int Fan = DmxData[5 + DmxOffset];
      if (Fan == 0) {
        Fan = 520;
      } else {
        targetTempData = map(Fan,1,256,30*8,80*8); // Temp range between 30-80C
      } 
      
      
      set_dmx(DmxData[1 + DmxOffset],
              DmxData[2 + DmxOffset], 
              DmxData[3 + DmxOffset], 
              DmxData[4 + DmxOffset],
              Fan);
      
      /*
      pwmValueRed   = current_calibration_mixed[0];
      pwmValueGreen = current_calibration_mixed[1];
      pwmValueBlue  = current_calibration_mixed[2];
      pwmValueWhite = current_calibration_mixed[3];
      
      pwmValueRed    = DmxData[1 + DmxOffset];
      pwmValueGreen  = DmxData[2 + DmxOffset];
      pwmValueBlue   = DmxData[3 + DmxOffset];
      pwmValueWhite  = DmxData[4 + DmxOffset];
      */
    } 
  }

  int bytesAvailable;
  now = millis();

  if (now - lastUpdate > 5000) {
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