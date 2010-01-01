/*
Abstract:
  Core parameters names

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PARAMETERS_H_DEFINED__
#define __CORE_PARAMETERS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Parameters
  {
    namespace Core
    {
      namespace Plugins
      {
        namespace Raw
        {
          const int64_t SCAN_STEP_DEFAULT = 1;
          const Char SCAN_STEP[] =
          {
            'z','x','t','u','n','e','.','c','o','r','e','.','p','l','u','g','i','n','s','.','r','a','w','.','s','c','a','n','_','s','t','e','p','\0'
          };
        }
      }
    }
  }
}
#endif //__IO_PROVIDERS_PARAMETERS_H_DEFINED__
