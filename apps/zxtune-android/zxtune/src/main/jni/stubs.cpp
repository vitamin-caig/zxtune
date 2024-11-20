/**
 *
 * @file
 *
 * @brief Different compiler-dependend stubs
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <stddef.h>

extern "C" char __cxa_demangle(const char*, char*, size_t*, int* status)
{
  if (status)
  {
    *status = -1;
  }
  return 0;
}
