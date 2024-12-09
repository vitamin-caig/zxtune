#ifndef F2POINT_H
#define F2POINT_H

/*
inheritance
object
	-> F2Point
*/
struct F2Point {
	int frame;
	int value;
};

void F2Point_defaults(struct F2Point* self);
/* default value for both ints: 0 */
void F2Point_ctor(struct F2Point* self, int x, int y);
struct F2Point* F2Point_new(int x, int y);

#endif
