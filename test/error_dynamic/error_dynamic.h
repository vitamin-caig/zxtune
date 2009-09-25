#ifndef __ERROR_DYNAMIC_H__
#define __ERROR_DYNAMIC_H__

#include <error.h>

extern "C"
{
  __attribute__ ((visibility("default"))) Error ReturnErrorByValue();
  void ReturnErrorByReference(Error&);
}

#endif
