#ifndef FLOD_INTERNAL_H
#define FLOD_INTERNAL_H

#include <string.h>
#include <stdlib.h>

/* convention
 * 
 * as3 static initializers   -> classname_defaults(self)
 *                              1. calls zeromem
 *                              implemented thru CLASS_DEF_INIT
 *                              2. then sets all static initializers.
 *                              manual
 *                              ideally after (1) sets float (Number) members to NAN as well.
 * 
 * as3 constructor           -> classname_ctor(self, ...)
 *                              1. calls classname_defaults
 *                              implemented thru CLASS_CTOR_DEF
 *                              2. does whatever the as3 constructor did.
 *                              manual
 *                              ... refers to the as3 constructor arguments.
 * 
 * C "constructor" (dynamic) -> classname_new(...)
 *                              1. allocates new object
 *                              2. runs classname_ctor(obj, ...)
 *                              entirely implemented thru CLASS_NEW_BODY
 *                              
 * C "constructor" (stack)   -> allocate object on stack
 *                              then call classname_ctor(obj, ...) 
 * 
 
 * every class shall implement all 3 functions mentioned above,
 * being stubs if the as3 functionality was unused.
 * 
 * 
void xxx_defaults(struct xxx* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void xxx_ctor(struct xxx* self) {
	CLASS_CTOR_DEF(xxx);
	// original constructor code goes here
}

struct xxx* xxx_new(void) {
	CLASS_NEW_BODY(xxx);
}
 
 */

#define NEW(classname) struct classname* self = malloc(sizeof(*self))

/* gcc specific macro */
#define CLASS_NEW_BODY(classname, args...) \
	NEW(classname); \
	if(self) classname ## _ctor(self , ##args); \
	return (self)

#define CLASS_CTOR_DEF(classname) \
	classname ## _defaults(self);

#define CLASS_DEF_INIT() do { memset(self, 0, sizeof(*self)); } while(0)



#endif
