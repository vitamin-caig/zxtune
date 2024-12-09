#ifndef FLOD_H
#define FLOD_H

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../include/debug.h"
#include "../../flashlib/Common.h"
#include "../../flashlib/ByteArray.h"

#define PFUNC_QUIET
#ifndef PFUNC_QUIET
#  define PFUNC() fprintf(stderr, "%s\n", __FUNCTION__)
#else
#  define PFUNC() do { } while(0)
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define STRSZ(X) X , sizeof(X) - 1
#define is_str(chr, lit) (!memcmp(chr, STRSZ(lit)))

#endif