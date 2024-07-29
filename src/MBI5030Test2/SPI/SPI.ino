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
}
void loop() {
    //send code value 1, 128, 254
    sendByte(1);
    sleep(0.3);
    sendByte(128);
    sleep(0.3);
    sendByte(254);
    sleep(0.3);
}

void sendByte(uint8_t data) {
    for (int i = 7; i >= 0; --i) {
        digitalWrite(DATA_CLK, LOW);
        delayMicroseconds(5);
        if ((data >> i & 1))
            digitalWrite(DATA_OUT, HIGH);
        else
            digitalWrite(DATA_OUT, LOW);
        digitalWrite(DATA_CLK, HIGH);
        delayMicroseconds(5);
    }
}
