#ifdef __cplusplus
extern "C" {
#endif

#define VULED8_MODE_DOT 1
#define VULED8_MODE_BAR 2

struct vuled8 {
	int32_t dmate;
	int32_t dmate_count;
	int32_t v0;
	int32_t v1;
	int32_t v2;
	uint32_t rval;
	uint32_t hold;
	uint16_t hold_decr;
	uint16_t srate;
	uint32_t amult;
};

int
vuled8_init(struct vuled8 *vu);

int
vuled8_set_thresh_ref(struct vuled8 *vu,
		int vuref);

int
vuled8_add_sample(struct vuled8 *vu,
		uint32_t aval);

int
vuled8_set_bits(struct vuled8 *vu,
		uint8_t *bits,
		uint32_t val,
		int mode);

#ifdef __cplusplus
}
#endif
