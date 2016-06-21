#include <string.h>
#include <stdint.h>

#include "Arduino.h"

#include "vuled8.h"

#define xVULED8_DEBUG

#define SETLSBS(x) (((x) << 1) - 1)

/* num samples to hold peak
 */
#define HOLD_COUNT 4000

/* +20 dBu measured on oscilliscope
 * then vals computed in spreadsheet
 */
uint32_t THRESH_20DBU[8] = {
	53074, //  +6 dBu
	37573, //  +3 dBu
	26600, //   0 dBu (rel to +20 dBu)
	18831, //  -3 dBu
	13332, //  -6 dBu
	 8412, // -10 dBu
	 4730, // -15 dBu
	 2660  // -20 dBu
};

int
vuled8_set_thresh_ref(struct vuled8 *vu,
		int vuref)
{
	switch (vuref) {
		case 0:
			vu->amult = 0;
			break;
		case 1: //   0 dBu
		default:
			vu->amult = 1000;
			break;
		case 2: //  +4 dBu
			vu->amult = 631;
			break;
		case 3: // +12 dBu
			vu->amult = 251;
			break;
		case 4: // +20 dBu
			vu->amult = 100;
			break;
	}

	return 0;
}

#define DMATE 32

int
vuled8_init(struct vuled8 *vu)
{
	memset(vu, 0, sizeof(*vu));

	vu->amult = 0;
	vu->dmate = 0;
	vu->dmate_count = DMATE;
	vu->v0 = 0;
	vu->v1 = 0;
	vu->v2 = 0;
	vu->rval = 0;
	vu->hold = 0;
	vu->hold_decr = HOLD_COUNT;
	vu->srate = 0;

	vuled8_set_thresh_ref(vu, 0); // off for now
}

#ifdef VULED8_DEBUG

#define MON_PERIOD 5000
#define DSIZE 64

uint32_t adata[DSIZE];
uint32_t rdata[DSIZE];
uint32_t pdata[DSIZE];
uint32_t hdata[DSIZE];

int di = 0;

uint32_t mon_last_time = 0;

#endif

extern uint32_t ctime;

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

	// scale values to dBu reference (and to compensate for precision)
	aval *= vu->amult;

	vu->dmate = (vu->dmate * (DMATE - 1) + aval) / DMATE;
	if (vu->dmate_count-- == 0) {
		vu->dmate_count = DMATE;

		vu->v0 = vu->v1;
		vu->v1 = vu->v2;
		vu->v2 = (vu->dmate - 93 * vu->v0 + 218 * vu->v1) / 128;
		vu->rval = (vu->v0 + vu->v2) + 2 * vu->v1;
	}

	// rst down to curr peak || set hold to new peak
	if (vu->hold_decr-- == 0 || vu->rval > vu->hold) {
		vu->hold = vu->rval;
		vu->hold_decr = HOLD_COUNT;
	}

#ifdef VULED8_DEBUG
if (ctime > mon_last_time + MON_PERIOD) {
	if (di < DSIZE) {
		adata[di] = aval;
		rdata[di] = vu->rval;
		pdata[di] = vu->dmate;
		hdata[di] = vu->hold;
		di++;
	} else {
		Serial.println(vu->srate); // actually sample time in us

		for (di = 0; di < DSIZE; di++) {
			Serial.print(adata[di]);
			Serial.print('\t');
		}
		Serial.println();
		for (di = 0; di < DSIZE; di++) {
			Serial.print(rdata[di]);
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
#endif

	return 0;
}

int
vuled8_set_bits(struct vuled8 *vu,
		uint8_t *bits,
		uint32_t val,
		int mode)
{
	int ti;

	for (ti = 0; ti < 8 && val < THRESH_20DBU[ti]; ti++) {
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
