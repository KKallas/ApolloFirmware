#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>
#include <limits.h>

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

                                    // DMX
const int DmxReceivePin = 26;       // ESP32 pin (SK led Pin)
int DmxOffset = 0;                  // Offset from 512 addresses
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
        // Red calibartion for flux loss at high temperature, pad temp data from 11 bits to 16 bits
        compRedVal = calculateRedColor(pwmValueRed, currentTempData>>3, false);
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

  Serial.print("* Welcome to Apollo lamp to use the terminal:\n");
  Serial.print("* - For the 'A' command:\n");
  Serial.print("*   - Extracts color channel and fan intensity values from the UART data (A%i %i %i %i %i) -> RGBWF\n");
  Serial.print("*   - Updates the corresponding color intensity values and fan intensity based on extracted values.\n");
  Serial.print("* - For the 'D' command:\n");
  Serial.print("*   - Sets predefined values for color channel and fan intensity.\n");
  Serial.print("* - For the 'T' command:\n");
  Serial.print("*   - Prints the current temperature value to Serial.\n");
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
      pwmValueRed   = DmxData[1 + DmxOffset]*8;
      pwmValueGreen = DmxData[2 + DmxOffset]*8;
      pwmValueBlue  = DmxData[3 + DmxOffset]*8;
      pwmValueWhite = DmxData[4 + DmxOffset]*8;
      // DMX fan mode if 0 the 1024 else the actual value
      int DmxFanValue = DmxData[5 + DmxOffset]*8;
      if (DmxFanValue == 0) {
        pwmValueFan = 2048;
      }
      else {
        pwmValueFan = DmxFanValue;
      }
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
}