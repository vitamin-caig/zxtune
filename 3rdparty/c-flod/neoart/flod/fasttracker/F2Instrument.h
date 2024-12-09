#ifndef F2INSTRUMENT_H
#define F2INSTRUMENT_H

#include "F2Sample.h"
#include "F2Data.h"


#define F2INSTRUMENT_MAX_SAMPLES 1

// fixed
#define F2INSTRUMENT_MAX_NOTESAMPLES 96

/*
inheritance
object
	-> F2Instrument
*/
struct F2Instrument {
	//name         : String = "",
	char *name;
	//samples      : Vector.<F2Sample>,
	struct F2Sample samples[F2INSTRUMENT_MAX_SAMPLES];
	int noteSamples[F2INSTRUMENT_MAX_NOTESAMPLES];
	//noteSamples  : Vector.<int>,
	int fadeout;
	struct F2Data volData;
	int volEnabled;
	struct F2Data panData;
	int panEnabled;
	int vibratoType;
	int vibratoSweep;
	int vibratoSpeed;
	int vibratoDepth;
};

void F2Instrument_defaults(struct F2Instrument* self);
void F2Instrument_ctor(struct F2Instrument* self);
struct F2Instrument* F2Instrument_new(void);

#endif
