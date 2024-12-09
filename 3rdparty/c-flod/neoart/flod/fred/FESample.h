#ifndef FESAMPLE_H
#define FESAMPLE_H

//fixed
#define FESAMPLE_MAX_ARPEGGIO 16

struct FESample {
	//arpeggio      : Vector.<int>,
	int arpeggio[FESAMPLE_MAX_ARPEGGIO];
	int pointer;
	int loopPtr;
	int length;
	int relative;
	int type;
	int synchro;
	int envelopeVol;
	int attackSpeed;
	int attackVol;
	int decaySpeed;
	int decayVol;
	int sustainTime;
	int releaseSpeed;
	int releaseVol;
	int arpeggioLimit;
	int arpeggioSpeed;
	int vibratoDelay;
	int vibratoDepth;
	int vibratoSpeed;
	int pulseCounter;
	int pulseDelay;
	int pulsePosL;
	int pulsePosH;
	int pulseSpeed;
	int pulseRateNeg;
	int pulseRatePos;
	int blendCounter;
	int blendDelay;
	int blendRate;
};

void FESample_defaults(struct FESample* self);
void FESample_ctor(struct FESample* self);
struct FESample* FESample_new(void);

#endif
