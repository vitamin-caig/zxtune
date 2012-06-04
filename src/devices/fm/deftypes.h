
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
/*																			  */
/*							   RAINE DATATYPES								  */
/*																			  */
/******************************************************************************/

#ifndef __DEFTYPES_H__
#define __DEFTYPES_H__

#if defined(RAINE_DOS) || defined(RAINE_UNIX)

#define DEF_INLINE		inline

#else

#define DEF_INLINE		__inline

#endif

#if (!defined( RAINE_WIN32 )) || defined(__MINGW32__)

#define UINT8	unsigned char	   //  8-bit unsigned
#define UINT16	unsigned short	   // 16-bit unsigned

#define INT8	signed char 	   //  8-bit signed
#define INT16	short			   // 16-bit signed

#ifndef __int3264 // collision with windows api !!!
#define UINT32	unsigned int	   // 32-bit unsigned
#define UINT64	unsigned long long // 64-bit unsigned

#define INT32	int 			   // 32-bit signed
#define INT64	long long		   // 64-bit signed
#endif

#define CPTR	unsigned int	   // 32-bit pointer

#define BOOL	int

#else

#if _MSC_VER > 1000
#pragma warning( disable : 4005 4057 4127 4131 4142 4146 4152 4232 4245 4706 4710 4711 4761 )
// C4005 - 'identifier' : macro redefinition
// C4057 - 'operator' : 'identifier1' indirection to slightly different base types from 'identifier2'
// C4127 - conditional expression is constant
// C4131 - 'function' : uses old-style declarator
//		   --- This one is needed since the unzip (zlib) declares using ANSI standard (old style).
// C4142 - benign redefinition of type
// C4146 - unary minus operator applied to unsigned type, result still unsigned
// C4152 - non standard extension, function/data ptr conversion in expression
//		   --- This one is needed for almost all of the files, when MSVC is put on warning level 4,
//			   It complains about almost every hard coded string in the source complaining about typecast
//			   required etc.  It also complains on function pointer assignments that are not typecasted correctly.
// C4232 - nonstandard extension used : 'identifier' : address of dllimport 'dllimport' is not static, identity not guaranteed
// C4245 - 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
// C4706 - assignment within conditional expression
//		   --- For cleanliness these should actually be split out into multiple lines, also makes
//			   debugging these lines a lot easier.	Most occurances of this were testing memory allocation.
// C4710 - function 'function' not inlined
// C4711 - function 'function' selected for inline expansion
// C4761 - integral size mismatch in argument : conversion supplied
//		   --- This one occurs for a few of the libraries that are used due to parameter type mis-match.
//			   The compiler does however straigten these out, but the warning is issued so it can be resolved
//			   correctly and no compiler stupidity can break the program.
#endif // _MSC_VER > 1000

typedef unsigned char UINT8;   //  8-bit unsigned
typedef unsigned short UINT16; // 16-bit unsigned
typedef unsigned int UINT32;   // 32-bit unsigned
typedef __int64 UINT64; 	   // 64-bit unsigned

typedef signed char INT8;	   //  8-bit signed
typedef signed short INT16;    // 16-bit signed
typedef signed int INT32;	   // 32-bit signed
typedef __int64 INT64;		   // 64-bit signed

typedef unsigned int	CPTR;	  // 32-bit pointer

typedef int BOOL;

#endif

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

#ifndef MIN
#define MIN(x,y)	 (((x) < (y)) ? (x) : (y))
#define MAX(x,y)	 (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)	 MAX((x), MIN((y), (z)))
#endif

#define ASCII_ID(a,b,c,d)	  ((a<<24) | (b<<16) | (c<<8) | (d<<0))

typedef void mapper_direct(int bank, int cols);

#endif

#ifdef __cplusplus
}
#endif

#define DLL_EXPORT extern "C" __declspec (dllexport)