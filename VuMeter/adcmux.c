#include <stdint.h>

#include "Arduino.h"

#include "adcmux.h"

const uint8_t C_ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIF) | ADCMUX_PRESCALER;
const uint8_t C_ADMUX = (1 << 6);

uint8_t atmega32u_adcmap[12] = {
	0b000111, // A0 -> ADC7
	0b000110, // A1 -> ADC6
	0b000101, // A2 -> ADC5
	0b000100, // A3 -> ADC4
	0b000001, // A4 -> ADC1
	0b000000, // A5 -> ADC0
	// A6-A11 not supported (yet)
	0b100000, // A6 -> ADC8
	0b100010, // A7 -> ADC10
	0b100011, // A8 -> ADC11
	0b100100, // A9 -> ADC12
	0b100101, // A10 -> ADC13
	0b100001, // A11 -> ADC9
};

uint8_t atmega32u_pinmap[6] = { 7, 6, 5, 4, 1, 0 };

int
adcmux_init(struct adcmux *am, struct adcmux_chan *chans, int nchan)
{
	memset(am, 0, sizeof(*am));

	am->step = 1;
	am->ci = 0;  // chan index
	am->rci = 0; // chan index actually reading from
	am->nchan = nchan & 0xff;
	am->chans = chans;

	ADCSRB = ADHSM | 0x00;
	ADMUX = C_ADMUX | atmega32u_adcmap[chans[0].pin];
	ADCSRA = C_ADCSRA | (1 << ADSC);

	// disable digital inputs
	uint8_t didr0 = 0x00;
	int ci;
	for (ci = 0; ci < nchan; ci++) {
		didr0 |= (1 << atmega32u_pinmap[chans[ci].pin]);
	}
	DIDR0 = didr0;

	return 0;
}

int
adcmux_read(struct adcmux *am, int *ci)
{
	uint32_t aval;
	uint8_t nci;
	int done;

	done = 0;
	nci = 0;

	// read sample
	while (!(ADCSRA & (1 << ADIF)));

	*ci = am->rci;

	do {
		if (am->ci == am->nchan) {
			am->ci = 0;
			am->step++; // 0 % 0 = 0 but whatev
		}

		if ((am->step % am->chans[am->ci].modu) == 0) {
			nci = am->ci;
			done = 1;
		}

		am->ci++;
	} while (!done);

	if (am->rci != nci) {
		// change chan
		// TODO: add support for MUX5 bit of ADCSRA so
		// that we can get to A6-A11
		ADMUX = C_ADMUX | atmega32u_adcmap[am->chans[nci].pin];
		am->rci = nci;
	}

	ADCSRA = C_ADCSRA;
	aval = ADCL & 0xff;
	aval |= (ADCH & 0x3) << 8;

	return aval;
}

