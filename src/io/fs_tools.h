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

#include <types.h>

namespace ZXTune
{
  namespace IO
  {
    String ExtractFirstPathComponent(const String& path, String& restPart);
    String AppendPath(const String& path1, const String& path2);
    //void SplitFSName(const String& fullpath, String& dir, String& filename, String& subname);
    //String BuildNameTemplate(const String& fullpath, const String& templ, const StringMap& properties);
  }
}

#endif //__IO_FS_TOOLS_H_DEFINED__
