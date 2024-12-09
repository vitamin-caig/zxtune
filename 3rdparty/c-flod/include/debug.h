#ifndef DEBUG_H
#define DEBUG_H

#define empty_body do {} while(0)
#if defined(__x86_64) || defined(__x86_64__)
#  define INTEL_CPU 64
#elif defined(__x86) || defined(__x86__) || defined(__i386) || defined(__i386__)
#  define INTEL_CPU 32
#endif

//#define NO_BREAKPOINTS
#ifdef NO_BREAKPOINTS
#  define breakpoint() empty_body
#else
#  ifdef INTEL_CPU
#    define breakpoint() __asm__("int3")
#  else
#    include <signal.h>
#    include <unistd.h>
#    define breakpoint() kill(getpid(), SIGTRAP)
#  endif
#endif

//#define NO_ASSERT
#ifdef NO_ASSERT
#  define assert_dbg(exp) empty_body
#  define assert_lt(exp) empty_body
#  define assert_lte(exp) empty_body
#  define assert(x) empty_body
#else
#  include <stdio.h>
#  define assert_dbg(exp) do { if (!(exp)) breakpoint(); } while(0)
#  define assert_op(val, op, max) do { if(!((val) op (max))) { \
    fprintf(stderr, "[%s:%d] %s: assert failed: %s < %s\n", \
    __FILE__, __LINE__, __FUNCTION__, # val, # max); \
    breakpoint();}} while(0)

#  define assert_lt (val, max) assert_op(val, <, max)
#  define assert_lte(val, max) assert_op(val, <= ,max)
#endif

#endif