/* These are normally defined in the Amiga's types library, but are
   defined here instead to ensure portability with minimal changes to the
   original Amiga source-code
*/

#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

#include <limits.h>

typedef unsigned short uint16;
typedef unsigned char	uint8;
typedef signed short	int16;
typedef signed char	int8;

#if INT_MAX == 32767
typedef unsigned long	uint32;
typedef signed long	int32;
#else
typedef unsigned int uint32;
typedef signed int int32;
#endif

typedef double float64;
typedef char TEXT;
typedef int BOOL;
typedef long LONG;
typedef unsigned long ULONG;
#define FALSE 0
#define TRUE 1

#endif
