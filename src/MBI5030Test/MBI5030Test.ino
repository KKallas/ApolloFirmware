#include <Arduino.h>
#include "MBI5030.h"

#define BRIGHTNESS_MAX 65535
#define BRIGHTNESS_STEPSIZE 256
#define FADE_DELAY 1

MBI5030 chip1(GPIO_NUM_21, GPIO_NUM_18, GPIO_NUM_4, GPIO_NUM_12);	// spi_out, spi_in, spi_clk, spi_latch
uint16_t pwm_data[16];

void setup() {
  Serial.begin(115200);
	memset(pwm_data, 0x00, sizeof(pwm_data)); // fill 32 bytes with 0s
	chip1.spi_init();
  chip1.write_config(0x0000 | PWM_16BIT, 0xFF, 1);
  chip1.update(pwm_data, 1);
}

void loop() {
	uint8_t counter;
	uint16_t brightness;
  	Serial.printf("%i\n", counter);

	brightness = 0;
	while (brightness <= BRIGHTNESS_MAX) {

		for (counter = 0; counter <= 15; counter++) {
			pwm_data[counter] = brightness;
		}

		chip1.update(pwm_data, 1);

		if (brightness <= BRIGHTNESS_MAX - BRIGHTNESS_STEPSIZE) {
			brightness += BRIGHTNESS_STEPSIZE;
		} else if (brightness == BRIGHTNESS_MAX) {
			break;
		} else {
			brightness = BRIGHTNESS_MAX;
		}
		delay(FADE_DELAY);
	}

	while (brightness >= 0) {

		for (counter = 0; counter <= 15; counter++) {
			pwm_data[counter] = brightness;
		}

		chip1.update(pwm_data, 1);

		if (brightness >= 0 + BRIGHTNESS_STEPSIZE) {
			brightness -= BRIGHTNESS_STEPSIZE;
		} else if (brightness == 0) {
			break;
		} else {
			brightness = 0;
		}
		delay(FADE_DELAY);
	}
	delay(250);
}