#include <string.h>
#include <stdint.h>

#include "Arduino.h"

#include "vuled8.h"

#define SETLSBS(x) (((x) << 1) - 1)

/* num samples to hold peak
 */
#define HOLD_COUNT 4000

/* Filter coefficents for 2nd order LP (x1000)
 */
#define FC_A 490
#define FC_B 490
#define FC_C 20

/* +12 dBu measured on oscilliscope
 * then others computed in spreadsheet
 */
const uint32_t THRESH_12DBU[8] = {
	2729, //  +6 dBu
	1932, //  +3 dBu
	1367, //   0 dBu (rel to +12 dBu)
	 968, //  -3 dBu
	 685, //  -6 dBu
	 432, // -10 dBu
	 243, // -15 dBu
	 137  // -20 dBu
};

int
vuled8_set_thresh_ref(struct vuled8 *vu,
		int vuref)
{
	int ri;

	for (ri = 0; ri < 8; ri++) {
		switch (vuref) {
			case 0: // off
				vu->thresh[ri] = (uint32_t)-1;
				break;
			case 1: //   0 dBu
			default:
				vu->thresh[ri] = (THRESH_12DBU[ri] * 100) / 398;
				break;
			case 2: //  +4 dBu
				vu->thresh[ri] = (THRESH_12DBU[ri] * 100) / 251;
				break;
			case 3: // +12 dBu
				vu->thresh[ri] = THRESH_12DBU[ri];
				break;
		}
	}

	return 0;
}

int
vuled8_init(struct vuled8 *vu)
{
	memset(vu, 0, sizeof(*vu));
	vu->peak = 0;
	vu->avg0 = 0;
	vu->avg = 0;
	vu->hold = 0;
	vu->hold_decr = HOLD_COUNT;
	vu->srate = 0;

	vuled8_set_thresh_ref(vu, 0); // off for now
}

#define MON_PERIOD 5000
#define DSIZE 64

uint32_t rdata[DSIZE];
uint32_t adata[DSIZE];
uint32_t pdata[DSIZE];
uint32_t hdata[DSIZE];
int di = 0;

extern uint32_t ctime;
uint32_t mon_last_time = 0;

#define SRATE_COUNT_OVER 0xfff
uint16_t srate_count = 0;
uint32_t srate_last_time = 0;

int
vuled8_add_sample(struct vuled8 *vu,
		uint32_t aval)
{
	if (srate_count == SRATE_COUNT_OVER) {
		srate_count = 0;
		vu->srate = (ctime - srate_last_time) * 1000 / SRATE_COUNT_OVER;
		srate_last_time = ctime;
	}
	srate_count++;

	// rectify
	if (aval >= 511) {
		aval -= 511;
	} else {
		aval = 511 - aval;
	}

	/* most values scaled to compensate for loss of
	 * precision and rounding errors
	 */
	aval *= 100;

	/* This *should* be 2nd order LP filter to 8 Hz
	 * but coefficients are currently just a wild guess
	 */
	uint32_t tmp = vu->avg;
	vu->avg = (FC_A * vu->avg + FC_B * vu->avg0 + FC_C * aval) / 1000;
	vu->avg0 = tmp;

	// rst down to curr peak || set hold to new peak
	if (vu->hold_decr-- == 0 || vu->avg > vu->hold) {
		vu->hold = vu->avg;
		vu->hold_decr = HOLD_COUNT;
	}

/*
if (ctime > mon_last_time + MON_PERIOD) {
	if (di < DSIZE) {
		rdata[di] = aval;
		adata[di] = vu->avg;
		pdata[di] = vu->hold_decr;
		hdata[di] = vu->hold;
		di++;
	} else {

		Serial.println(vu->srate); // actually sample time in us

		for (di = 0; di < DSIZE; di++) {
			Serial.print(rdata[di]);
			Serial.print('\t');
		}
		Serial.println();
		for (di = 0; di < DSIZE; di++) {
			Serial.print(adata[di]);
			Serial.print('\t');
		}
		Serial.println();
		for (di = 0; di < DSIZE; di++) {
			Serial.print(pdata[di]);
			Serial.print('\t');
		}
		Serial.println();
		for (di = 0; di < DSIZE; di++) {
			Serial.print(hdata[di]);
			Serial.print('\t');
		}
		Serial.println();

		di = 0;
		mon_last_time = ctime;
	}
}
*/

	return 0;
}

int
vuled8_set_bits(struct vuled8 *vu,
		uint8_t *bits,
		uint32_t val,
		int mode)
{
	int ti;

	for (ti = 0; val < vu->thresh[ti] && ti < 8; ti++) {
		;
	}
	if (ti < 8) {
		if (mode == VULED8_MODE_BAR) {
			*bits |= SETLSBS(1 << (7 - ti));
		} else {
			*bits |= 1 << (7 - ti);
		}
	}

	return 0;
}
