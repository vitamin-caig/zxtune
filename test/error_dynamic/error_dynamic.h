#ifndef __ERROR_DYNAMIC_H__
#define __ERROR_DYNAMIC_H__

#include <error.h>
#include <api_dynamic.h>

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
