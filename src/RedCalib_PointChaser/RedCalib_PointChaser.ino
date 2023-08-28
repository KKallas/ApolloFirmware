#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>

const uint taskCore = 0;
const uint pwmPin = 21;
unsigned long lastUpdate = millis();
unsigned long now = millis();
                                    // LIGHT
const uint pwmFrequency = 20000;    // 20 kHz
const uint pwmResolution = 11;      // 11-bit resolution (2048 levels)
const uint blinkFrequency = 200;    // Update rate
int redPwmValue   = 0;             // Per channel output
int greenPwmValue = 0;
int bluePwmValue  = 0;
int whitePwmValue = 0;
int fanPwmValue   = 0;


const uint STEPS   = 50;
int redTarget     = 0;             // Next value after interpolation
int greenTarget   = 0;
int blueTarget    = 0;
int whiteTarget   = 0;
int fanTarget     = 0;

int redStep        = 0;             // Steps at next render loop, if target reached step = 0
int greenStep      = 0;
int blueStep       = 0;
int whiteStep      = 0;
int fanStep        = 0;

uint currentTempData = 0;           // Temp sensor data RAW (multiply by 0.125 to get Celsius)
uint TempArray[5] = {200, 200, 200, 200, 200}; // Initialize with default value 25*8 temp sensor RAW value

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
      // Point chaser calculations
      updatePointChasers();
      // Red calibartion for flux loss at high temperature
      int compRedVal = calculateRedColor(redPwmValue, currentTempData>>3);
      ledcWrite(0, compRedVal);
      ledcWrite(1, greenPwmValue);
      ledcWrite(2, bluePwmValue);
      ledcWrite(3, whitePwmValue);
      ledcWrite(4, fanPwmValue*8);
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}

/**
 * Update the current value towards a target value with a specified step.
 *
 * This function updates the current value towards a target value by applying a specified step. It ensures
 * that the current value approaches the target value by modifying the current value in increments of the step.
 * The step can be positive or negative, determining the direction of the update. The function handles cases
 * where the step would overshoot the target value, ensuring that the current value reaches the target value
 * without exceeding it.
 *
 * @param currentValue The current value to be updated.
 * @param targetValue The target value to which the current value should be updated.
 * @param step The step value that determines the rate of change towards the target value.
 * @return The updated current value after applying the step.
 */
long updateChannel(long currentValue, long targetValue, long step) {
  long nextValue = currentValue + step;
  if (step < 0) {
    if (nextValue <= targetValue) {
      step = 0;
      currentValue = targetValue;
    }
  } else {
    if (nextValue >= targetValue) {
      step = 0;
      currentValue = targetValue;
    }
  }
  currentValue = currentValue + step;
  return currentValue;
}

void updatePointChasers() {
  redPwmValue = updateChannel(redPwmValue, redTarget, redStep);
  greenPwmValue = updateChannel(greenPwmValue, greenTarget, greenStep);
  bluePwmValue = updateChannel(bluePwmValue, blueTarget, blueStep);
  whitePwmValue = updateChannel(whitePwmValue, whiteTarget, whiteStep);
  fanPwmValue = updateChannel(fanPwmValue, fanTarget, fanStep);
  //Serial.printf("\"redPwm\":%i,\"greenPwm\":%i,\"bluePwm\":%i,\"whitePwm\":%i,\"fanPwm\":%i\n", redPwmValue, greenPwmValue, bluePwmValue, whitePwmValue, fanPwmValue);
  //Serial.printf("\"redStep\":%i,\"greenStep\":%i,\"blueStep\":%i,\"whiteStep\":%i,\"fanStep\":%i\n", redStep, greenStep, blueStep, whiteStep, fanStep);
}

void updateColor(int redIn, int greenIn, int blueIn, int whiteIn, int fanIn) {
  redStep = calculateStep(redIn, redTarget);
  greenStep = calculateStep(greenIn, greenTarget);
  blueStep = calculateStep(blueIn, blueTarget);
  whiteStep = calculateStep(whiteIn, whiteTarget);
  fanStep = calculateStep(fanIn, fanTarget);

  redTarget = redIn;
  greenTarget = greenIn;
  blueTarget = blueIn;
  whiteTarget = whiteIn;
  fanTarget = fanIn;
}

void rgbt2rgbw(uint redIn, uint greenIn, uint blueIn, uint tempIn, uint fan) {
  // first 5600K ranges
  // 100%  1780,1855, 410,1900
  // 50%    895,1010, 221, 980
  // 0.1%     6,   3,   1,   6
  uint minValue = 0;
  uint vals[3] = {redIn,greenIn,blueIn};

  // Get brightness
  for (int i = 1; i < 3; i++) {
    if (vals[i] < minValue) {
      minValue = vals[i];
    }
  }

  uint intensity = minValue;
  uint outR = 0;
  uint outG = 0;
  uint outB = 0;
  uint outW = 0;
  uint outF = 0;

  // Convert RGB to RGBW @ 5600K
  if(intensity > 1024) {
    outR = map(intensity,0,1024,6,895);
    outG = map(intensity,0,1024,3,1010);
    outB = map(intensity,0,1024,1,221);
    outW = map(intensity,0,1024,6,980);
  } else {
    outR = map(intensity,1024,2048,895,1780);
    outG = map(intensity,1024,2048,1010,1855);
    outB = map(intensity,1024,2048,221,410);
    outW = map(intensity,1024,2048,980,1900);
  }

  // Add color
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
      redPwmValue   = DmxData[1 + DmxOffset]*8;
      greenPwmValue = DmxData[2 + DmxOffset]*8;
      bluePwmValue  = DmxData[3 + DmxOffset]*8;
      whitePwmValue = DmxData[4 + DmxOffset]*8;
      // DMX fan mode if 0 the 1024 else the actual value
      int DmxFanValue = DmxData[5 + DmxOffset];
      if (DmxFanValue == 0) {
        fanPwmValue = 2048;
      }
      else {
        fanPwmValue = DmxFanValue*8;
      }
      Serial.print(DmxFanValue);
    } 
  }

  int bytesAvailable;
  now = millis();

  if (now - lastUpdate > 1000) {
    // TODO: Read temp
    
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
      // Color Channels (R,G,B,W)
      IntensityRed   = twoByteChar(UartReceivedChars[1],UartReceivedChars[2]);
      IntensityGreen = twoByteChar(UartReceivedChars[3],UartReceivedChars[4]);
      IntensityBlue  = twoByteChar(UartReceivedChars[5],UartReceivedChars[6]);
      IntensityWhite = twoByteChar(UartReceivedChars[7],UartReceivedChars[8]);
      // Fan
      Fan = UartReceivedChars[9];

      updateColor(IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan);

      // UART answer
      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f,\"redStep\":%i*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125, redStep);
      //Serial.printf("dmx_fan:%01X,temp:%f\n", Fan, currentTempData*0.125);
      
      UartNewData = false;
    }
    if (UartNewData == true && UartReceivedChars[0] == 'D') {
      // Color Channels (R,G,B,W)
      IntensityRed   = twoByteChar(0,1);
      IntensityGreen = twoByteChar(0,1);
      IntensityBlue  = twoByteChar(0,1);
      IntensityWhite = twoByteChar(0,1);
      // Fan
      Fan = 0x88;

      Serial.printf("\"red_val\":%i,\"green_val\":%i,\"blue_val\":%i,\"white_val\":%i,\"fan_val\":%i,\"temp\":%f*\n", IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan, currentTempData*0.125);
      updateColor(IntensityRed, IntensityGreen, IntensityBlue, IntensityWhite, Fan);
      
      UartNewData = false;
    }
  }

uint readTemp() {
  int temperatureData;

  Wire.requestFrom(78,2);
  while(Wire.available()) {
    temperatureData = (Wire.read() << 8) | Wire.read();  // fill out 2 bytes from I2C comm
    temperatureData = temperatureData >> 5;              // adjust to 11 bit value
    insertIntoTempArray(temperatureData);
    return TempArray[2];                                 // return the middle item to avoid extremes
  }
}

void insertIntoTempArray(int newValue) {
  // Find the position to insert the new value
  int insertIndex = 4;    // Start from the last position (Arry len 5)
  
  while (insertIndex >= 0 && newValue < TempArray[insertIndex]) {
    if (insertIndex < 4) {
      TempArray[insertIndex + 1] = TempArray[insertIndex]; // Shift elements to the right
    }
    insertIndex--;
  }
  
  // Insert the new value at the correct position
  TempArray[insertIndex + 1] = newValue;
}

int twoByteChar(char byte1,char byte2) {
  uint output = ((uint)byte1 << 8) | (uint)(byte2 & 0xFF);
  return(output);
}