#include "M5Dial.h"
#include <math.h>
#include <EEPROM.h>
#define I2C_SLAVE_ADDRESS 8 // I2C address of the slave

int currentSel  = 0;
int currValue[] = {             0,  2800,    0,      0,     0,         0,            1,            0,         0};
int maxVal[]    = {           255, 10000,  255,    255,   255,       511,            1,         8500,     10000};
int minVal[]    = {             0,  2800,    0,      0,     0,         0,            0,        -3000,         0};
String labels[] = {   "Intensity",  "WB","Red","Green","Blue","DMX addr", "DMX enable", "Lamp temp.", "Fan RPM"};
String lut[]    = {"OFF", "ON"};
bool editMode   = true;
bool screenUpdate = true;
int receivedData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Array to store received data
int sentData[9]     = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Data to send

// Timers
long lastI2cUpdate;
long lastI2cSent;
long lastTurn;
long lastScreenUpdate;

// Encoder positions
int newPosition;
int lastPosition;
int adjustment;

void setup() {
  // Setup the M5 Encoder
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);

  // Create Encoder handeler thread
  xTaskCreatePinnedToCore(
                EncoderThread,     // Function to implement the task
                "EncoderHandeler", // Name of the task
                10000,             // Stack size in words
                NULL,              // Task input parameter
                0,                 // Priority of the task
                NULL,              // Task handle.
                0);                // Core where the task should run [0 empty core, 1 admin core]

  // Setup I2C master
  Wire.begin();

  // Storage
  EEPROM.begin(512);
  EEPROM.get(0,currValue);
}

void EncoderThread(void * pvParameters) {
  // for the direction filter to know, what direction was the last change that came through
  bool lastDirection = true;
  while(true) {
    // Read value changes
    M5Dial.update();
    newPosition = M5Dial.Encoder.readAndReset();

    // Encoder input
    if(newPosition != 0) {
      long newTurn = millis();
      // filter out direction change to take at least .3 seconds to filter the incorrect direction changes
      bool currentDirection = (newPosition >= 0); 
      bool allowed = ((newTurn - lastTurn < 300) && (currentDirection == lastDirection)) 
                     || (newTurn - lastTurn > 300);
      if(allowed) {
        // Set correct previous direction for input filtering
        lastDirection = currentDirection;
        int comp = 1;

        // turn speed variable exponent
        if(newTurn - lastTurn < 80) comp = 2;
        if(newTurn - lastTurn < 50) comp = 10;
        if(newTurn - lastTurn < 30) {
          newPosition = lastPosition;
          M5Dial.Encoder.readAndReset();
        }
        // if reading was unreliable(too quick) just repeat the last reading value
        lastPosition = newPosition;

        // compansate read in value if apllicable
        // compensation will only work only in intensity and RGB controlls
        if(editMode == true && (currentSel==0 || currentSel==2 || currentSel==3 || currentSel==4)) {newPosition *= comp;}

        // Debug serial
        // TODO: Fix debug format
        //Serial.printf("[%i/%i]Val from spinner: %i\n", newTurn - lastTurn, comp, newPosition);
        adjustment += newPosition;

        // Update screen and lamp
        M5Dial.Speaker.tone(1000, 10);

        lastTurn = newTurn;
        screenUpdate = true;
      }
    }

    // Handle button to toggle edit mode
    if (M5Dial.BtnA.wasPressed()) {
      // If in edit mode go to change menu page mode
      if (editMode) {
        editMode = false;
      } else {
        // The "Lamp Temp" and "Fan RPM" are read only and the layout will not enable edit mode
        if(currentSel < 7) {
          editMode = true;
        }
      }
      // Update screen
      screenUpdate = true;
    }

    // Long press example for preset values
    /*
    if (M5Dial.BtnA.pressedFor(5000)) {
        M5Dial.Encoder.write(100);
    }
    */
  }
}

void loop() {
    // I2C input @ 0.5s
    if(millis() - lastTurn > 500) {
      lastTurn = millis();
      // keep the old value to check if updated value is different
      int tempVal = currValue[currentSel];
      // get the new value 2x as the 1st time it will the old value in i2c buffer ready to read
      requestColorFromSlave(true);
      requestColorFromSlave(false);
      // if the current visible value was updated then call screen update as well
      // TODO Current val VS IIC val
      if(tempVal != currValue[currentSel]) {
        screenUpdate = true;
        Serial.printf("DEBUG: %i:%i:%i\n", tempVal, currValue[currentSel], currValue[1]);
      }
    }

    // If new value, send to I2C and redraw screen
    if(screenUpdate) {
      redrawScreen();
      Serial.printf("DEBUG: %i\n", currValue[1]);
      sendColorToController();

      // Save Current value
      EEPROM.put(0, currValue);
      EEPROM.commit();

      // Postopne the I2C update if the button was used to change the values
      lastTurn = millis();
      screenUpdate = false;
    }
    // makes the encoder readout much more reliable
    usleep(1000); 
}

void redrawScreen() {
  if(millis() - lastScreenUpdate < 50) {delay(50);}
  lastScreenUpdate = millis();
  M5Dial.Display.clear();
  M5Dial.Display.setTextSize(2);
  if (editMode) {
    int multplier = 1;
    if(currentSel == 1) {multplier = 100;}
    // If editing menuitem
    currValue[currentSel] = currValue[currentSel] + (adjustment*multplier);
    // handle min/max value
    if(currValue[currentSel] < minVal[currentSel]) {currValue[currentSel] = minVal[currentSel];}
    if(currValue[currentSel] > maxVal[currentSel]) {currValue[currentSel] = maxVal[currentSel];}
    // If intensity
    if(currentSel == 0) {
      // If in intecity change also the RGB sliders
      // int = int((R+G+B)/3)
      for(int i = 2; i<5; i++) {
        currValue[i] = currValue[i] + adjustment;
        if(currValue[i] > maxVal[i]) {currValue[i] = maxVal[2];}
        if(currValue[i] < minVal[i]) {currValue[i] = minVal[2];}
      }
    }
    // Menu looparound
    if(currentSel > 1 && currentSel < 8) {
      int smallest = currValue[2];
      if(currValue[3] < currValue[2]) {smallest = currValue[3];}
      if(currValue[4] < currValue[3]) {smallest = currValue[3];}
      currValue[0] = smallest;
    }
    M5Dial.Display.drawLine(70, M5Dial.Display.height() / 2-10, M5Dial.Display.width()-70, M5Dial.Display.height() / 2-10, GREEN);
    M5Dial.Display.drawLine(70, M5Dial.Display.height() / 2+50, M5Dial.Display.width()-70, M5Dial.Display.height() / 2+50, GREEN);
  } 
  // Change menu page mode
  else {
    // If controlling menuitem
    if(currentSel + adjustment < 0) {
      currentSel = 9;
    }
    currentSel = (currentSel + adjustment) % 9;
    M5Dial.Display.drawLine(30, M5Dial.Display.height() / 2-15, M5Dial.Display.width()-30, M5Dial.Display.height() / 2-15, GREEN);
    M5Dial.Display.drawLine(30, M5Dial.Display.height() / 2-65, M5Dial.Display.width()-30, M5Dial.Display.height() / 2-65, GREEN);
  }

  // Draw value for DMX ON/OFF
  if(currentSel == 6) {
    if(currValue[currentSel] == 0) {
      M5Dial.Display.drawString(lut[0], M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
    } else {
      M5Dial.Display.drawString(lut[1], M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
    }
  // Draw value for lamp temp
  } else if(currentSel == 7) {
    M5Dial.Display.drawString(String(int(round(currValue[currentSel]/8))), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
  // Draw value for anything else
  } else {
    M5Dial.Display.drawString(String(currValue[currentSel]), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
  }

  M5Dial.Display.setTextSize(1);
  M5Dial.Display.drawString(labels[currentSel], M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 40);

  adjustment = 0;
}