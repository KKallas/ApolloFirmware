#include "M5Dial.h"
#include <math.h>
#include <EEPROM.h>
#define I2C_SLAVE_ADDRESS 8 // I2C address of the slave

long oldPosition = -999;
int currentSel = 0;
int currValue[] = {          0,  2800,    0,      0,     0,         0,            1,            0,         0};
int maxVal[] = {           255, 10000,  255,    255,   255,       511,            1,         8500,     10000};
int minVal[] = {             0,  2800,    0,      0,     0,         0,            0,        -3000,         0};
String labels[] = {"Intensity",  "WB","Red","Green","Blue","DMX addr", "DMX enable", "Lamp temp.", "Fan RPM"};
String lut[] = {"OFF", "ON"};
bool editMode = true;
int receivedData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Array to store received data
int sentData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Data to send
int diffCount=0;
String colorString = "Green";

long lastI2cUpdate;
long lastI2cSent;

long newPosition;

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);

    xTaskCreatePinnedToCore(
                    I2cComm,       // Function to implement the task
                    "I2cCommHandeler",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    0);             // Core where the task should run [0 empty core, 1 admin core]

    // Setup I2C master
    Wire.begin();

    // Storage
    EEPROM.begin(512);
    EEPROM.get(0,currValue);
}

void loop() {
    M5Dial.update();
    newPosition = M5Dial.Encoder.readAndReset();

    if (M5Dial.BtnA.wasPressed()) {
      //TODO if DMX is signal is incoming then it is not possible to change numbers so only edit mode in evrywhere else but enable dmx
      if (editMode) {
        editMode = false;
      } else {
        editMode = true;
      }
    }
    if (newPosition != oldPosition || M5Dial.BtnA.wasPressed()) {
      // update throug i2c
      // leave 50ms if it is quicker ill get Kernel panic sometimes
      if(millis()>lastI2cSent+50) {
        M5Dial.Speaker.tone(1000, 10);
        sendColorToController();
        lastI2cSent = millis();
        redrawScreen();

        // Save Current value
        EEPROM.put(0, currValue);
        EEPROM.commit();

        oldPosition = newPosition;
      }   
    }   
    /*
    if (M5Dial.BtnA.pressedFor(5000)) {
        M5Dial.Encoder.write(100);
    }
    */
}

void redrawScreen() {
  M5Dial.Display.clear();
  M5Dial.Display.setTextSize(2);
  if (editMode) {
    int multplier = 1;
    if(currentSel == 1) {multplier = 100;}
    // If editing menuitem
    currValue[currentSel] = currValue[currentSel] + (newPosition*multplier);
    // handle min value
    if(currValue[currentSel] < minVal[currentSel]) {currValue[currentSel] = minVal[currentSel];}
    // handle max value
    if(currValue[currentSel] > maxVal[currentSel]) {currValue[currentSel] = maxVal[currentSel];}
    // If intensity
    if(currentSel == 0) {
      // If in intecity change also the RGB sliders
      for(int i = 2; i<5; i++) {
        currValue[i] = currValue[i] + newPosition;
        if(currValue[i] > maxVal[i]) {currValue[i] = maxVal[2];}
        if(currValue[i] < minVal[i]) {currValue[i] = minVal[2];}
      }
    }
    // Menu looparound
    if(currentSel > 1 && currentSel < 5) {
      int smallest = currValue[2];
      if(currValue[3] < currValue[2]) {smallest = currValue[3];}
      if(currValue[4] < currValue[3]) {smallest = currValue[3];}
      currValue[0] = smallest;
    }
    M5Dial.Display.drawLine(70, M5Dial.Display.height() / 2-10, M5Dial.Display.width()-70, M5Dial.Display.height() / 2-10, GREEN);
    M5Dial.Display.drawLine(70, M5Dial.Display.height() / 2+50, M5Dial.Display.width()-70, M5Dial.Display.height() / 2+50, GREEN);
  } else {
    // If controlling menuitem
    if(currentSel + newPosition < 0) {
      currentSel = 7;
    }
    currentSel = (currentSel + newPosition) % 7;
    Serial.println("Menu item: " + String(currentSel));
    M5Dial.Display.drawLine(30, M5Dial.Display.height() / 2-15, M5Dial.Display.width()-30, M5Dial.Display.height() / 2-15, GREEN);
    M5Dial.Display.drawLine(30, M5Dial.Display.height() / 2-65, M5Dial.Display.width()-30, M5Dial.Display.height() / 2-65, GREEN);
  }

  M5Dial.Display.drawString(String(currValue[currentSel]), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);

  M5Dial.Display.setTextSize(1);
  M5Dial.Display.drawString(labels[currentSel], M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 40);
}