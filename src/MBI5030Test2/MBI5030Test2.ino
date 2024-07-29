#include <Arduino.h>

#define HIGH 1
#define LOW  0

#define DATA_IN  21
#define DATA_OUT 18
#define DATA_CLK 19
#define GRY_CLK  4
#define GRY_LE   12



void setup() {
    pinMode(DATA_IN, INPUT);
    pinMode(DATA_OUT, OUTPUT);
    pinMode(DATA_CLK, OUTPUT);
    pinMode(GRY_CLK, OUTPUT);
    pinMode(GRY_LE, OUTPUT);

    // Create PWM clock on GRY_CLK
    ledcSetup(0, 300000, 8); // last stable value through testing
    ledcAttachPin(GRY_CLK, 0);
    ledcWrite(0, 0x88);

    Serial.begin(115200);
}

void loop() {
    Serial.println("Sent low");
    for(uint8_t i=0; i<15; i++) {
      sendBytes(0x0005, 1); // 0 - 14
    }
    sendBytes(0x0005, 2);  // 15

    sleep(1);

    Serial.println("Sent high");
    for(uint8_t i=0; i<15; i++) {
      sendBytes(0xFFFF-1, 1); // 0 - 14
    }
    sendBytes(0xFFFF-1, 2);  // 15
    sleep(1);
}

/**
 * Sends a single bit of data to the MBI5030/MBI5043 chip and waits for it to be received.
 *
 * @param data The value of the bit being sent (true = 1, false = 0)
 * @param leSignal Whether this is the last bit in the transmission (if true, also pulses GRY_LE high)
 * @return The received value of the bit
 */
bool sendAndRecieveBit(bool data, bool leSignal) {
  // Set the clock line low to start sending the data bit
  digitalWrite(DATA_CLK, LOW);
  
  // Wait for a short period before sending the actual data bit
  delayMicroseconds(5);  

  // Send the data bit (1 if true, 0 if false)
  digitalWrite(DATA_OUT, data ? HIGH : LOW); 
  
  // If this is the last bit in the transmission, pulse the GRY_LE line high
  if (leSignal) digitalWrite(GRY_LE, HIGH);
  else digitalWrite(GRY_LE, LOW);

  // Wait for a short period before checking the received value
  delayMicroseconds(5);
  
  // Set the clock line high to signal that the data bit has been sent
  digitalWrite(DATA_CLK, HIGH); 
  
  // Wait for a short period before checking the received value
  delayMicroseconds(10);
  
  // Return the received value of the bit (1 if high, 0 if low)
  bool input = digitalRead(DATA_IN);
  if(input) Serial.print('X');
  else Serial.print('O');
  return input;
}

void sendBytes(uint16_t val, uint8_t leLen) {
  uint16_t dataIn = 0; // variable to hold the final byte sent

  Serial.print('[');
  for (uint8_t i = 0; i < 16; i++) { 
    // Extract the current bit value from the 16-bit value
    bool bitValue = (val >> i) & 1;

    if(i > (15 - leLen)) dataIn |= uint16_t(sendAndRecieveBit(bitValue, true) << i);
    else dataIn |= uint16_t(sendAndRecieveBit(bitValue, false) << i);
  }
  digitalWrite(GRY_LE, LOW);
  digitalWrite(DATA_CLK, LOW);

  Serial.print("]:");
  Serial.print(String(dataIn));
  Serial.print("\n"); // print the final byte sent as a string
}
