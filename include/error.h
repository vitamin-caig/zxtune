/*
Abstract:
  Error subsystem definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is the part of Textator project
*/

#ifndef __ERROR_H_DEFINED__
#define __ERROR_H_DEFINED__

#include "types.h"

struct Error
{
  //Datatype for source text line identification
  //ErrorTag=FILE_TAG+__LINE__
  //assume __LINE__ < 65536
  typedef unsigned ErrorTag;

#ifndef NDEBUG
  struct Detail
  {
    Detail() : Tag(0), File(0), Function(0), Line(0)
    {
    }
    Detail(ErrorTag tag, const char* file, const char* function, std::size_t line)
      : Tag(tag), File(file), Function(function), Line(line)
    {
    }
    ErrorTag Tag;
    const char* File;
    const char* Function;
    std::size_t Line;
  };
#else
  typedef ErrorTag Detail;
#endif

  Error() : Dtl(), Code(0), Text() //success
  {
  }
  Error(Detail dtl, std::size_t code) : Dtl(dtl), Code(code), Text()
  {
  }
  Error(Detail dtl, std::size_t code, const String& text) : Dtl(dtl), Code(code), Text(text)
  {
  }
  operator bool () const
  {
    return 0 != Code;
  }
  bool operator ! () const
  {
    return 0 == Code;
  }
  Detail Dtl;
  std::size_t Code;
  String Text;
};

#define MKTAG1(a) 0x ## a
#define MKTAG(a) MKTAG1(a)
#define MAKETAG (MKTAG(FILE_TAG) + __LINE__)
#ifndef NDEBUG
  #define ERROR_DETAIL (Error::Detail(MAKETAG, __FILE__, __FUNCTION__, __LINE__))
#else
  #define ERROR_DETAIL (Error::Detail(MAKETAG))
#endif

#endif //__ERROR_H_DEFINED__
