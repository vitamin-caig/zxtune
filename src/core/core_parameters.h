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

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Core
    {
      namespace AYM
      {
        //! Chip type, integer, every bit is type for specific chip
        const Char TYPE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','y','p','e','\0'
        };
        //! Frequency table name for ay-based plugins, string or dump
        const Char TABLE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','a','b','l','e','\0'
        };
      }        
    }
  }
}

#endif //__CORE_PARAMETERS_H_DEFINED__
