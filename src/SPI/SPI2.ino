#include <Arduino.h>

#define HIGH 1
#define LOW  0

// Double check pins
#define DATA_IN  21
#define DATA_OUT 18
#define DATA_CLK 19
#define GRY_CLK  4
#define GRY_LE   12 // has pulldown marked on drawing

void setup() {
    pinMode(DATA_IN, INPUT);
    pinMode(DATA_OUT, OUTPUT);
    pinMode(DATA_CLK, OUTPUT);
    pinMode(GRY_CLK, OUTPUT); 
    pinMode(GRY_LE, OUTPUT); // CS
}