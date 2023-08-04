#include <Arduino.h>
#include <esp_dmx.h>

int receivePin = 15;
dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();

void setup() {
  Serial.begin(115200);
  dmx_set_pin(dmxPort, 0, receivePin, 0);
  dmx_driver_install(dmxPort, DMX_DEFAULT_INTR_FLAGS);
}

void loop() {
  dmx_packet_t packet;
  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK)) {
    unsigned long now = millis();
    if (!packet.err) {
      /* If this is the first DMX data we've received, lets log it! */
      if (!dmxIsConnected) {
        Serial.println("DMX is connected!");
        dmxIsConnected = true;
      }

      dmx_read(dmxPort, data, packet.size);

      if (now - lastUpdate > 1000) {
        /* Print the received start code - it's usually 0. */
        Serial.printf("Start code is 0x%02X and slot 1 is 0x%02X\n", data[0],
                      data[1]);
        lastUpdate = now;
      }
    } else {
      Serial.println("A DMX error occurred.");
    }
  } else if (dmxIsConnected) {
    /* If DMX times out after having been connected, it likely means that the
      DMX cable was unplugged. When that happens in this example sketch, we'll
      uninstall the DMX driver. */
    Serial.println("DMX was disconnected.");
    dmx_driver_delete(dmxPort);

    /* Stop the program. */
    while (true) yield();
  }
}
