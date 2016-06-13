#ifdef __cplusplus
extern "C" {
#endif

/* ADC prescaler values sets ADC frequency
 * Default is 16 MHz / 128 = 125 kHz but can run faster
 */
#define ADCMUX_PS_16 0b100   //  ~13us
#define ADCMUX_PS_32 0b101   //  ~24us
#define ADCMUX_PS_64 0b110   //  ~52us
#define ADCMUX_PS_128 0b111  // ~104us

#define ADCMUX_PRESCALER ADCMUX_PS_32

struct adcmux_chan {
	uint8_t pin;
	uint16_t modu;
};
struct adcmux {
	uint16_t step;
	uint8_t ci;
	uint8_t rci;
	uint8_t nchan;
	struct adcmux_chan *chans;
};

int adcmux_init(struct adcmux *am, struct adcmux_chan *chans, int num_chans);
int adcmux_read(struct adcmux *am, int *chi);

#ifdef __cplusplus
}
#endif
