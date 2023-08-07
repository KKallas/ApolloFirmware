#include <Arduino.h>
#include <esp_dmx.h>
#include <Wire.h>

const int taskCore = 0;
const int pwmPin = 4;
const int pwmFrequency = 20000;     // 20 kHz
const int pwmResolution = 11;       // 11-bit resolution (2048 levels)
const int blinkFrequency = 200;

const int receivePin = 15;
const dmx_port_t dmxPort = 1;
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();

int pwmValue1 = 0;

void ColorUpdate( void * pvParameters ){
 
    String taskMessage = "Running color updater on: ";
    taskMessage = taskMessage + xPortGetCoreID();

    ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
    ledcAttachPin(pwmPin, 0);       // Attach PWM to IO4 (Channel 0)

    while(true){                    // Alternately set the PWM duty cycle to pwmValue1 and pwmValue2, Half-period delay
      ledcWrite(0, pwmValue1*8);
      delayMicroseconds(1000000 / (blinkFrequency)); 
    }
 
}
 
void setup() {
 
  Serial.begin(115200);
 
  Serial.print("Creating services ");
  Serial.println(taskCore);

  pinMode(27, OUTPUT);              // Set IO27 as output
  digitalWrite(27, HIGH);           // Enable output on IO27 (set it to HIGH)  

  dmx_set_pin(dmxPort, -1, 26, -1);
  dmx_driver_install(dmxPort, DMX_DEFAULT_INTR_FLAGS);
 
  Wire.begin(23,22);                     // As we aure using default pins 21 & 22

  xTaskCreatePinnedToCore(
                    ColorUpdate,    // Function to implement the task
                    "ColorUpdate",  // Name of the task
                    10000,          // Stack size in words
                    NULL,           // Task input parameter
                    0,              // Priority of the task
                    NULL,           // Task handle.
                    taskCore);      // Core where the task should run
 
  Serial.println("Task created...");
  Serial.println("Starting main loop...");
}
 
void loop() {
  // Get the DMX value to debug
  int temperatureData;
  int bytesAvailable;
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
      pwmValue1 = data[512];

      if (now - lastUpdate > 1000) {
        //sleep(250);
        Wire.requestFrom(78,2);
        while(Wire.available()) {
          temperatureData = (Wire.read() << 8) | Wire.read();
          temperatureData = temperatureData >> 5;
        }

        /* Print the received start code - it's usually 0. */
        Serial.printf("%i Start code is 0x%02X and slot 1 is 0x%02X and temp is %f\n", lastUpdate, data[0], data[512],temperatureData*0.125);
        lastUpdate = now;
        
      }
    } 
    else {
        Serial.println("A DMX error occurred.");
    }
  } 
  else if (dmxIsConnected) {
    /* If DMX times out after having been connected, it likely means that the
    DMX cable was unplugged. When that happens in this example sketch, we'll
    uninstall the DMX driver. */
    Serial.println("DMX was disconnected.");
    dmx_driver_delete(dmxPort);

    /* Stop the program. */
    while (true) yield();
  }
}