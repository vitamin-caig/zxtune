/*
Abstract:
  Different io-related tools declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_FS_TOOLS_H_DEFINED__
#define __IO_FS_TOOLS_H_DEFINED__

#include <string_type.h>

namespace ZXTune
{
  namespace IO
  {
    String ExtractFirstPathComponent(const String& path, String& restPart);
    String AppendPath(const String& path1, const String& path2);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
