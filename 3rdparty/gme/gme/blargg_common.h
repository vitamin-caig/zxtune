// Sets up common environment for Shay Green's libraries.
// To change configuration options, modify blargg_config.h, not this file.

#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#undef BLARGG_COMMON_H
// allow blargg_config.h to #include blargg_common.h
#include "blargg_config.h"
#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H

// BLARGG_RESTRICT: equivalent to restrict, where supported
#if __GNUC__ >= 3 || _MSC_VER >= 1100
	#define BLARGG_RESTRICT __restrict
#else
	#define BLARGG_RESTRICT
#endif

// STATIC_CAST(T,expr): Used in place of static_cast<T> (expr)
#ifndef STATIC_CAST
	#define STATIC_CAST(T,expr) ((T) (expr))
#endif

#if !defined(_MSC_VER) || _MSC_VER >= 1910
	#define blaarg_static_assert(cond, msg) static_assert(cond, msg)
#else
	#define blaarg_static_assert(cond, msg) assert(cond)
#endif

// blargg_err_t (0 on success, otherwise error string)
#ifndef blargg_err_t
	typedef const char* blargg_err_t;
#endif

// blargg_vector - very lightweight vector of POD types (no constructor/destructor)
template<class T>
class blargg_vector {
	T* begin_;
	size_t size_;
public:
	blargg_vector() : begin_( 0 ), size_( 0 ) { }
	~blargg_vector() { free( begin_ ); }
	size_t size() const { return size_; }
	T* begin() const { return begin_; }
	T* end() const { return begin_ + size_; }
	blargg_err_t resize( size_t n )
	{
		void* p = realloc( begin_, n * sizeof (T) );
		if ( !p && n )
			return "Out of memory";
		begin_ = (T*) p;
		size_ = n;
		return 0;
	}
	void clear() { free( begin_ ); begin_ = nullptr; size_ = 0; }
	T& operator [] ( size_t n ) const
	{
		assert( n <= size_ ); // <= to allow past-the-end value
		return begin_ [n];
	}
};

// Use to force disable exceptions for allocations of a class
#include <new>
#ifndef BLARGG_DISABLE_NOTHROW
	#define BLARGG_DISABLE_NOTHROW \
		void* operator new ( size_t s ) noexcept { return malloc( s ); }\
		void* operator new ( size_t s, const std::nothrow_t& ) noexcept { return malloc( s ); }\
		void operator delete ( void* p ) noexcept { free( p ); }\
		void operator delete ( void* p, const std::nothrow_t&) noexcept { free( p ); }
#endif

// Use to force disable exceptions for a specific allocation no matter what class
#define BLARGG_NEW new (std::nothrow)

// BLARGG_4CHAR('a','b','c','d') = 'abcd' (four character integer constant)
#define BLARGG_4CHAR( a, b, c, d ) \
	((a&0xFF)*0x1000000L + (b&0xFF)*0x10000L + (c&0xFF)*0x100L + (d&0xFF))

#define BLARGG_2CHAR( a, b ) \
	((a&0xFF)*0x100L + (b&0xFF))

// BLARGG_COMPILER_HAS_BOOL: If 0, provides bool support for old compiler. If 1,
// compiler is assumed to support bool. If undefined, availability is determined.
#ifndef BLARGG_COMPILER_HAS_BOOL
	#if defined (__MWERKS__)
		#if !__option(bool)
			#define BLARGG_COMPILER_HAS_BOOL 0
		#endif
	#elif defined (_MSC_VER)
		#if _MSC_VER < 1100
			#define BLARGG_COMPILER_HAS_BOOL 0
		#endif
	#elif defined (__GNUC__)
		// supports bool
	#elif __cplusplus < 199711
		#define BLARGG_COMPILER_HAS_BOOL 0
	#endif
#endif
#if defined (BLARGG_COMPILER_HAS_BOOL) && !BLARGG_COMPILER_HAS_BOOL
	// If you get errors here, modify your blargg_config.h file
	typedef int bool;
	const bool true  = 1;
	const bool false = 0;
#endif

// blargg_long/blargg_ulong = at least 32 bits, int if it's big enough

#if INT_MAX < 0x7FFFFFFF || LONG_MAX == 0x7FFFFFFF
	typedef long blargg_long;
#else
	typedef int blargg_long;
#endif

#if UINT_MAX < 0xFFFFFFFF || ULONG_MAX == 0xFFFFFFFF
	typedef unsigned long blargg_ulong;
#else
	typedef unsigned blargg_ulong;
#endif

// int8_t etc.

// Apply minus sign to unsigned type and prevent the warning being shown
	template<typename T>
	inline T uMinus(T in)
	{
		return ~(in - 1);
	}

// TODO: Add CMake check for this, although I'd likely just point affected
// persons to a real compiler...
#if 1 || defined (HAVE_STDINT_H)
	#include <stdint.h>
#endif

#if __GNUC__ >= 3
	#define BLARGG_DEPRECATED __attribute__ ((deprecated))
#else
	#define BLARGG_DEPRECATED
#endif

// Use in place of "= 0;" for a pure virtual, since these cause calls to std C++ lib.
// During development, BLARGG_PURE( x ) expands to = 0;
// virtual int func() BLARGG_PURE( { return 0; } )
#ifndef BLARGG_PURE
	#define BLARGG_PURE( def ) def
#endif

#endif
#endif
