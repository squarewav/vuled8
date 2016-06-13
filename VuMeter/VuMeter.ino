#include <SPI.h>

#include "adcmux.h"
#include "vuled8.h"

#define PLT_PERIOD 100
#define LED_PERIOD 5
#define POT0_PERIOD 100

uint32_t ctime = 0;
uint32_t plt_last_time = 0;
uint32_t led_last_time = 0;
uint32_t pot0_last_time = 0;
int last_pot0 = 0;

uint32_t aval;
uint32_t pavg = 0;
int chi;

struct adcmux am;
struct adcmux_chan chans[2] = {
	{ 0, 1 },
	{ 1, 200 },
};

struct vuled8 vu0;
int use_peak_dot = 0;

void setup()
{
	Serial.begin(115200);

	adcmux_init(&am, chans, 2);

	vuled8_init(&vu0);

	pinMode(SS, OUTPUT);
	digitalWrite(SS, HIGH);

	SPI.begin();
}

void loop()
{
	ctime = millis();

	aval = adcmux_read(&am, &chi);

	// vu meter audio input
	if (chi == 0)
		vuled8_add_sample(&vu0, aval);

	// vu meter ref pot
	if (chi == 1) {
		pavg = ((pavg * 4) + aval) / 5;

/*
		if (ctime > plt_last_time + PLT_PERIOD) {
			plt_last_time = ctime;
Serial.println(pavg);
		}
*/
		if (ctime > pot0_last_time + POT0_PERIOD) {
			pot0_last_time = ctime;

			int pot0 = pavg / 114; // 9 segments

			if (pot0 != last_pot0) {
				last_pot0 = pot0;

				int vuref = pot0 / 3 + 1;

				if (pot0 == 0 || pot0 == 3 || pot0 == 6)
					vuref = 0; // VU off

				vuled8_set_thresh_ref(&vu0, vuref);

				use_peak_dot = pot0 == 2 || pot0 == 5 || pot0 == 8;
			}
		}
	}

	if (ctime > led_last_time + LED_PERIOD) {
		led_last_time = ctime;

		uint8_t ledbits = 0x00;

		if (use_peak_dot)
			vuled8_set_bits(&vu0, &ledbits, vu0.hold, VULED8_MODE_DOT);
		vuled8_set_bits(&vu0, &ledbits, vu0.avg, VULED8_MODE_BAR);

		digitalWrite(SS, LOW);

		// CAT4016 16 ch LED driver
		SPI.transfer(0);
		SPI.transfer(ledbits);

		digitalWrite(SS, HIGH);
	}
}
