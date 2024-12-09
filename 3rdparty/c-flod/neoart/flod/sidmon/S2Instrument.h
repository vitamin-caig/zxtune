#ifndef S2INSTRUMENT_H
#define S2INSTRUMENT_H

struct S2Instrument {
	unsigned short wave;
	unsigned short arpeggio;
	unsigned short vibrato;
	unsigned char waveLen;
	unsigned char waveDelay;
	unsigned char waveSpeed;
	unsigned char arpeggioLen;
	unsigned char arpeggioDelay;
	unsigned char arpeggioSpeed;
	unsigned char vibratoLen;
	unsigned char vibratoDelay;
	unsigned char vibratoSpeed;
	signed char pitchBend;
	unsigned char pitchBendDelay;
	unsigned char attackMax;
	unsigned char attackSpeed;
	unsigned char decayMin;
	unsigned char decaySpeed;
	unsigned char sustain;
	unsigned char releaseMin;
	unsigned char releaseSpeed;
};

void S2Instrument_defaults(struct S2Instrument* self);
void S2Instrument_ctor(struct S2Instrument* self);
struct S2Instrument* S2Instrument_new(void);

#endif
