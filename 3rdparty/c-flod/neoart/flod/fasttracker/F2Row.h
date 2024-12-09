#ifndef F2ROW_H
#define F2ROW_H

/*
inheritance
object
	-> F2Row
*/
struct F2Row {
	int note;
	int instrument;
	int volume;
	int effect;
	int param;
};

void F2Row_defaults(struct F2Row* self);
void F2Row_ctor(struct F2Row* self);
struct F2Row* F2Row_new(void);

#endif
