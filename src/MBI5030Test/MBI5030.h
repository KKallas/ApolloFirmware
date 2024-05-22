//
// To get a suitably high GSCLK frequency, the CLKO-FUSE of the ATMega168/328 has been programmed
// It will output it's system clock on PB0 ("digital pin" #8)
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

#ifndef _MBI5030_H_
#define _MBI5030_H_

// MBI5030 configuration regiter options
// First line of each #define-block: default value

// Thermal error flag (read)
// Register bit: E
// - UNTESTED -
#define THERMAL_ERROR 0x4000

// PWM resolution (read/write)
// Register bit: D
// - WORKS -
#define PWM_16BIT 0x0000
#define PWM_12BIT 0x2000

// PWM mode: normal / scramble (read/write)
// Register bit: C
// - WORKS - 
#define PWM_MODE_NORMAL	  0x0000
#define PWM_MODE_SCRAMBLE 0x1000

// DATA SYNC mode: auto / manual (read/write)
// Register bit: A
// - UNTESTED -
#define DATA_SYNC_AUTO    0x0000
#define DATA_SYNC_MANUAL  0x0400

// Thermal protection (read/write)
// Register bit: 1
// - UNTESTED -
#define THERMAL_PROTECTION_OFF 0x0000
#define THERMAL_PROTECTION_ON  0x0002

// Missing grayscale-clock shutdown
// Register bit: 0
// - UNTESTED -
#define MISSING_GSCLK_DET_ON  0x0000
#define MISSING_GSCLK_DET_OFF 0x0001

extern "C" {
#include <stdint.h>
}

class MBI5030 {
 public:
 	MBI5030(gpio_num_t spi_out_pin, gpio_num_t spi_in_pin, gpio_num_t spi_clk_pin,gpio_num_t spi_latch_pin);
	void spi_init(void);
	void update(uint16_t * pwm_data, uint8_t n);
	uint16_t read_error_report(void);
	uint16_t read_config(void);
	void write_config(uint16_t config_mask, uint8_t current_gain, uint8_t n);

// private:
	void spi_clk_high(void);
	void spi_clk_low(void);
	void spi_out_high(void);
	void spi_out_low(void);
	void spi_latch_high(void);
	void spi_latch_low(void);
	void pulse_spi_clk(void);

	void enable_error_detection(void);
	void prepare_error_report(void);
	uint16_t read_register(void);
	void prepare_config_read(void);
};

#endif
