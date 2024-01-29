/*

  DMX Read

  This sketch allows you to read DMX from a DMX controller using a standard DMX
  shield, such SparkFun ESP32 Thing Plus DMX to LED Shield. This sketch was
  made for the Arduino framework!

  Created 9 September 2021
  By Mitch Weisbrod

  https://github.com/someweisguy/esp_dmx

*/
#include <Arduino.h>
#include <esp_dmx.h>

/* First, lets define the hardware pins that we are using with our ESP32. We
  need to define which pin is transmitting data and which pin is receiving data.
  DMX circuits also often need to be told when we are transmitting and when we
  are receiving data. We can do this by defining an enable pin. */
int transmitPin = 26;
int receivePin = 32;
int enablePin = 0;
/* Make sure to double-check that these pins are compatible with your ESP32!
  Some ESP32s, such as the ESP32-WROVER series, do not allow you to read or
  write data on pins 16 or 17, so it's always good to read the manuals. */

/* Next, lets decide which DMX port to use. The ESP32 has either 2 or 3 ports.
  Port 0 is typically used to transmit serial data back to your Serial Monitor,
  so we shouldn't use that port. Lets use port 1! */
dmx_port_t dmxPort = 1;

/* Now we want somewhere to store our DMX data. Since a single packet of DMX
  data can be up to 513 bytes long, we want our array to be at least that long.
  This library knows that the max DMX packet size is 513, so we can fill in the
  array size with `DMX_PACKET_SIZE`. */
byte data[DMX_PACKET_SIZE];

/* The last two variables will allow us to know if DMX has been connected and
  also to update our packet and print to the Serial Monitor at regular
  intervals. */
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();

int packetsPerSec;
int lastVal[4] = {0,0,0,0};

void setup() {
  /* Start the serial connection back to the computer so that we can log
    messages to the Serial Monitor. Lets set the baud rate to 115200. */
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1, 22, 19);

  /* Now we will install the DMX driver! We'll tell it which DMX port to use, 
    what device configure to use, and which interrupt priority it should have. 
    If you aren't sure which configuration or interrupt priority to use, you can
    use the macros `DMX_CONFIG_DEFAULT` and `DMX_INTR_FLAGS_DEFAULT` to set the
    configuration and interrupt to their default settings. */
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_driver_install(dmxPort, &config, DMX_INTR_FLAGS_DEFAULT);

  /* Now set the DMX hardware pins to the pins that we want to use and setup
    will be complete! */
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}


int currentVal[4];

void loop() {
  /* We need a place to store information about the DMX packets we receive. We
    will use a dmx_packet_t to store that packet information.  */
  dmx_packet_t packet;

  /* And now we wait! The DMX standard defines the amount of time until DMX
    officially times out. That amount of time is converted into ESP32 clock
    ticks using the constant `DMX_TIMEOUT_TICK`. If it takes longer than that
    amount of time to receive data, this if statement will evaluate to false. */
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    /* If this code gets called, it means we've received DMX data! */

    /* Get the current time since boot in milliseconds so that we can find out
      how long it has been since we last updated data and printed to the Serial
      Monitor. */
    unsigned long now = millis();
    unsigned long Serial2PrintTime = now;
    bool NewValue = false;
    
    /* We should check to make sure that there weren't any DMX errors. */
    if (!packet.err) {
      /* If this is the first DMX data we've received, lets log it! */
      if (!dmxIsConnected) {
        dmxIsConnected = true;
      }

      /* Don't forget we need to actually read the DMX data into our buffer so
        that we can print it out. */
      Serial2PrintTime = millis();
      dmx_read(dmxPort, data, packet.size);
      packetsPerSec = packetsPerSec + 1;
      

      for(int iloc = 0;iloc < 4;iloc++) {
        if (data[iloc+1] != lastVal[iloc]) {
          lastVal[iloc] = data[iloc+1];
          NewValue = true;
        }
      }
  
      if (NewValue) {
        Serial2.printf("DS02->%i:%i:%i:%i//0000:::\n", data[1], data[2], data[3], data[4]);
        Serial.println("new");
      }

      if (now - lastUpdate > 1000) {
        /* Print the received start code - it's usually 0. */
        Serial.printf("DS10->recieved [%i] packets in a sec, serial2 printed las time in [%i]ms//0000:::\n", packetsPerSec, Serial2PrintTime);
        Serial.printf("DS10->last packet [%i:%i:%i:%i]//0000:::\n", data[1], data[2], data[3], data[4]);
        packetsPerSec = 0;
        lastUpdate = now;
      }
    } 
  } 
}
