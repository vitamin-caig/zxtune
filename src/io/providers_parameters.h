/*
Abstract:
  Providers parameters names

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __IO_PROVIDERS_PARAMETERS_H_DEFINED__
#define __IO_PROVIDERS_PARAMETERS_H_DEFINED__

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace IO
    {
      //per-backend parameters
      namespace Providers
      {
        //parameters for file provider
        namespace File
        {
          //! Bytes. Files with size greater than this value will be open using memory mapping
          const IntType MMAP_THRESHOLD_DEFAULT = 16384;
          const Char MMAP_THRESHOLD[] = 
          {
            'z','x','t','u','n','e','.','i','o','.','p','r','o','v','i','d','e','r','s','.','f','i','l','e','.','m','m','a','p','_','t','h','r','e','s','h','o','l','d','\0'
          };
        }
      }
    }
  }
}
#endif //__IO_PROVIDERS_PARAMETERS_H_DEFINED__
