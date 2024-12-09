#include "../../../include/endianness.h"
#include <stdint.h>
#include <math.h>
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define denormalA(X) ((float)((X) + 0x1.8p23f) - 0x1.8p23f)
#define denormalB(X) ((float)((X) + 0x1p-103  ) - 0x1p-103  )
#define denormalC(X) ((float)((X) + 1.e-18f  ) - 1.e-18f  )
#define denormalZ(X) (X)
#define denormal(X) denormalB(X)

union float_repr { float __f; uint32_t __i; };
#define floatrepr(f) (((union __float_repr){ (float)(f) }).__i)

/* this one uses the original behaviour */
static inline int convert_sample16a(Number sample) {
	Number mul;
	if (unlikely(sample > 1.0)) return 32767;
	if (unlikely(sample < -1.0)) return -32768;
	mul = 32767.f;
	if(sample < 0.0) mul = 32768.f;
	return sample * mul;
}

/* this one uses the original behaviour, but a constant factor 
   according to my tests, it is the fastest variant. */
static inline int convert_sample16b(Number sample) {
	if (unlikely(sample > 1.0)) return 32767;
	if (unlikely(sample < -1.0))return -32767;
	return sample * 32767.f;
}

static inline int clip(int sample) {
	if(unlikely(sample > 32767)) return 32767;
	else if(unlikely(sample < -32767)) return -32768;
	return sample;
}

/* this one uses rounding which is supposed to be faster,
   but at least using the denormal hack its 4 times slower. */
static inline int convert_sample16r(Number sample) {
	int ret = lrintf(denormal(sample) * 32767.f);
	return clip(ret);
	//return clip(floatrepr((denormal(sample) * 32767.f) + 0x1.8p23f) & 0x3fffff );
}

#define convert_sample16 convert_sample16b

#ifdef HARDWARE_FROM_AMIGA
static inline void process_wave(struct Amiga* self, unsigned int size) {
#else
static inline void process_wave(struct Soundblaster* self, unsigned int size) {
#endif
	struct Sample *sample = &self->super.buffer[0];
	off_t avail = ByteArray_bytesAvailable(self->super.wave);
	int complete_flag = 0;
	unsigned int i;
	signed short *dest = (signed short*) &self->super.wave->start_addr[self->super.wave->pos];
	
	if(size * 4 > avail) {
		size = avail / 4;
		complete_flag = 1;
	}

	for (i = 0; i < size; ++i) {
#ifdef HARDWARE_FROM_AMIGA
		AmigaFilter_process(self->filter, self->model, sample);
#endif
		*dest++ = le16(convert_sample16(sample->l));
		*dest++ = le16(convert_sample16(sample->r));
		//self->super.wave->writeShort(self->super.wave, convert_sample16(sample->l));
		//self->super.wave->writeShort(self->super.wave, convert_sample16(sample->r));
		sample->l = sample->r = 0.0;
		sample = sample->next;
	}
	ByteArray_set_position_rel(self->super.wave, size * 4);
	if(complete_flag) CoreMixer_set_complete(&self->super, 1);
}
