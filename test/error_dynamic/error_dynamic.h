#ifndef __ERROR_DYNAMIC_H__
#define __ERROR_DYNAMIC_H__

#include <error.h>

#if defined(_WIN32)
#define PUBLIC_API_EXPORT __declspec(dllexport)
#define PUBLIC_API_IMPORT __declspec(dllimport)
#elif __GNUC__ > 3
#define PUBLIC_API_EXPORT __attribute__ ((visibility("default")))
#define PUBLIC_API_IMPORT
#else
#define PUBLIC_API_EXPORT
#define PUBLIC_API_IMPORT
#endif

#ifdef API_IMPL
#define API_ENTRY PUBLIC_API_EXPORT
#else
#define API_ENTRY
#endif

extern "C"
{
  API_ENTRY Error ReturnErrorByValue();
  API_ENTRY void ReturnErrorByReference(Error&);
}

#endif
