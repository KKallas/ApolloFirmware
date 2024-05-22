//
// To get a suitably high GSCLK frequency, the CLKO-FUSE of the ATMega168/328 has been programmed
// It will OUTPUT it's system clock on PB0 ("digital pin" #8)
//
// THIS REQUIRES AN ISP PROGRAMMER (or 2nd Arduino loaded with Arduino-ISP)
//
// Diecimila + ATmega168: lfuse: 0xBF
//                        hfuse: 0xDD
//                        efuse: 0x00 (or 0xF8)
//
// Uno + ATmega328:       lfuse: 0xBF
//                        hfuse: 0xDE
//                        efuse: 0x05 (or 0xFD)
//                        
// To revert to Arduino's default FUSE settings, change 0xBF to 0xFF.
//
// Use: "http://www.engbedded.com/fusecalc" and ".../arduino-XXX/hardware/arduino/boards.txt"
//
// Make sure to get the latest MBI5030 datasheet (at least version Jan. 2009)!
// 

//#include <avr/io.h>
#include <stdint.h>
#include "driver/gpio.h"
//#include <Arduino.h>
#include "MBI5030.h"
#define HIGH 1
#define LOW 0
gpio_num_t spi_out, spi_in, spi_clk, spi_latch ; 

MBI5030::MBI5030(gpio_num_t spi_out_pin, gpio_num_t spi_in_pin, gpio_num_t spi_clk_pin, gpio_num_t spi_latch_pin)
{
	spi_out = spi_out_pin;
	spi_in = spi_in_pin;
	spi_clk = spi_clk_pin;
	spi_latch = spi_latch_pin;
}

void MBI5030::spi_init(void)
{
	gpio_set_direction(spi_out, GPIO_MODE_OUTPUT);
	gpio_set_direction(spi_in, GPIO_MODE_OUTPUT);
	gpio_set_direction(spi_clk, GPIO_MODE_OUTPUT);
	gpio_set_direction(spi_latch, GPIO_MODE_OUTPUT);
}

void MBI5030::spi_out_high(void)
{
	gpio_set_level(spi_out, HIGH);
}

void MBI5030::spi_out_low(void)
{
	gpio_set_level(spi_out,LOW);
}

void MBI5030::spi_clk_high(void)
{
	gpio_set_level(spi_clk,HIGH);
}

void MBI5030::spi_clk_low(void)
{
	gpio_set_level(spi_clk,LOW);
}

void MBI5030::spi_latch_high(void)
{
	gpio_set_level(spi_latch,HIGH);
}

void MBI5030::spi_latch_low(void)
{
	gpio_set_level(spi_latch, LOW);
}

void MBI5030::pulse_spi_clk(void)
{
	if(spi_clk == HIGH){
		gpio_set_level(spi_clk,LOW);
		gpio_set_level(spi_clk,HIGH);
	}
	else{
		gpio_set_level(spi_clk,HIGH);	
		gpio_set_level(spi_clk,LOW);	
	}
}

void MBI5030::update(uint16_t * pwm_data, uint8_t n)
{
	uint8_t data_word;
	uint8_t data_word_bit;
	uint16_t pwm_data_tmp;
	uint8_t count = 0;
	//
	// measure how long one update-cycle takes
	//
	// uint32_t start;
	// uint32_t stop;
	// start = micros();

	// send the first 15 words with "data-latch"
	// the input shift register is 16bit wide
	// it requires a "data-latch" to move the data to
	// the chip-internal buffers

	for(int i = 0; i < 15; i++){
		for (data_word = n*count; data_word < n*count + n - 1 ; data_word++) {
			pwm_data_tmp = pwm_data[data_word];
		
			for (data_word_bit = 0; data_word_bit < 16; data_word_bit++) {
				// set or clear data - MSB first !
				if (pwm_data_tmp & 1000000000000000) {
					spi_out_high();
				} else {
					spi_out_low();
				}
				// pulse spi clock and shift temporary data by 1 to the left
				pulse_spi_clk();
				pwm_data_tmp <<= 1;
			}
		}
		pwm_data_tmp = pwm_data[n*count + n - 1];
			for (data_word_bit = 0; data_word_bit < 15; data_word_bit++) {
				// set or clear data - MSB first !
				if (pwm_data_tmp & 1000000000000000) {
					spi_out_high();
				} else {
					spi_out_low();
				}
				// pulse spi clock and shift temporary data by 1 to the left
				pulse_spi_clk();
				pwm_data_tmp <<= 1;
			}
			spi_latch_high();	// "data-latch" START

			if (pwm_data_tmp & 1000000000000000) {
				spi_out_high();
			} else {
				spi_out_low();
			}
			pulse_spi_clk();

			spi_latch_low();	// "data-latch" END

			count++;
	}

	// send the last word with "global-latch" 
	// to transfer the last word and make the data "live"
	count = 15;
	for (data_word = n*count; data_word < n*count + n - 1 ; data_word++) {
		pwm_data_tmp = pwm_data[data_word];
		
		for (data_word_bit = 0; data_word_bit < 16; data_word_bit++) {
				// set or clear data - MSB first !
			if (pwm_data_tmp & 1000000000000000) {
				spi_out_high();
			} else {
				spi_out_low();
			}
				// pulse spi clock and shift temporary data by 1 to the left
			pulse_spi_clk();
			pwm_data_tmp <<= 1;
		}
	}
		pwm_data_tmp = pwm_data[n*count + n - 1];
			for (data_word_bit = 0; data_word_bit < 13; data_word_bit++) {
				// set or clear data - MSB first !
				if (pwm_data_tmp & 1000000000000000) {
					spi_out_high();
				} else {
					spi_out_low();
				}
				// pulse spi clock and shift temporary data by 1 to the left
				pulse_spi_clk();
				pwm_data_tmp <<= 1;
			}

		if (pwm_data_tmp & 1000000000000000) {
			spi_out_high();
		} else {
			spi_out_low();
		}
		pulse_spi_clk();
		pwm_data_tmp <<= 1;

		spi_latch_high();	// "global-latch" START
		if (pwm_data_tmp & 1000000000000000) {
			spi_out_high();
		} else {
			spi_out_low();
		}
		pulse_spi_clk();
		pwm_data_tmp <<= 1;

		if (pwm_data_tmp & 1000000000000000) {
			spi_out_high();
		} else {
			spi_out_low();
		}
		pulse_spi_clk();

		spi_latch_low();	// "global-latch" END
////////////////////////////////////////////////////////////////////////////////	
	
	//
	// measure how long one update-cycle takes
	//      
	//stop = micros();
	//Serial.println(stop-start);
}
void MBI5030::write_config(uint16_t config_mask, uint8_t current_gain , uint8_t n) // n is no. of chips
{

	uint16_t current_gain_mask = (((uint16_t) (current_gain)) << 2);
	uint16_t config_data = (0x0000 | config_mask | current_gain_mask);
	uint8_t config_data_bit;
	uint16_t config_data_tmp;

	for(int i=1; i<=n-1 ; i++){
		config_data_tmp = config_data;	
		for (config_data_bit = 0; config_data_bit < 16; config_data_bit++) {
			if (config_data_tmp & 1000000000000000) {
				spi_out_high();
			} else {
				spi_out_low();
			}
			pulse_spi_clk();
			config_data_tmp <<= 1;
		}
	}	
	// send first 5 bits
	config_data_tmp = config_data;
	for (config_data_bit = 0; config_data_bit <= 4; config_data_bit++) {
		if (config_data_tmp & 1000000000000000) {
			spi_out_high();
		} else {
			spi_out_low();
		}
		pulse_spi_clk();
		config_data_tmp <<= 1;
	}

	spi_latch_high();
	// send bits 5..14
	for (config_data_bit = 5; config_data_bit <= 14; config_data_bit++) {
		if (config_data_tmp & 1000000000000000) {
			spi_out_high();
		} else {
			spi_out_low();
		}
		pulse_spi_clk();
		config_data_tmp <<= 1;
	}

	// send last bit
	if (config_data_tmp & 1000000000000000) {
		spi_out_high();
	} else {
		spi_out_low();
	}
	spi_clk_high();
	spi_latch_low();
	spi_clk_low();
}
////////////////////////////////////////////////////////////////////////////////
// void MBI5030::enable_error_detection(void)
// {
// 	spi_latch_high();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	spi_clk_high();
// 	spi_latch_low();
// 	spi_clk_low();
// 	delayMicroseconds(64);	// some time to stabilize readings
// }

// void MBI5030::prepare_error_report(void)
// {
// 	spi_latch_high();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	spi_clk_high();
// 	spi_latch_low();
// 	spi_clk_low();
// }

// uint16_t MBI5030::read_error_report(void)
// {
// 	enable_error_detection();
// 	prepare_error_report();
// 	return read_register();
// }

// uint16_t MBI5030::read_register(void)
// {
// 	uint16_t register_status = 0;
// 	uint8_t register_status_bit;

// 	// read bits 0-14
// 	for (register_status_bit = 0; register_status_bit <= 14;
// 	     register_status_bit++) {
// 		if (*_spi_in_PIN & _spi_in_pinmask) {
// 			register_status |= 1;
// 		} else {
// 			// already full with zeros
// 		}
// 		pulse_spi_clk();
// 		register_status <<= 1;
// 	}

// 	// read bit 15
// 	if (*_spi_in_PIN & _spi_in_pinmask) {
// 		register_status |= 1;
// 	} else {
// 		// already full with zeros
// 	}

// 	return register_status;
// }

// void MBI5030::prepare_config_read(void)
// {
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	spi_latch_high();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	pulse_spi_clk();
// 	spi_clk_high();
// 	spi_latch_low();
// 	spi_clk_low();
// }

// uint16_t MBI5030::read_config(void)
// {
// 	prepare_config_read();
// 	return read_register();
// }
//////////////////////////////////////////////////////////////////////////////////////////////////////////

