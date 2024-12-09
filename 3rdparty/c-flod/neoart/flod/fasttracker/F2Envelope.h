#ifndef F2ENVELOPE_H
#define F2ENVELOPE_H

/*
inheritance
object
	-> F2Envelope
*/
struct F2Envelope {
	int value;
	int position;
	int frame;
	int delta;
	int fraction;
	int stopped;
};

void F2Envelope_defaults(struct F2Envelope* self);
void F2Envelope_ctor(struct F2Envelope* self);
struct F2Envelope* F2Envelope_new(void);

void F2Envelope_reset(struct F2Envelope* self);

#endif
