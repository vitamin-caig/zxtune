#ifndef __FS_TOOLS_H_DEFINED__
#define __FS_TOOLS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace IO
  {
    void SplitFSName(const String& fullpath, String& dir, String& filename, String& subname);
    String BuildNameTemplate(const String& fullpath, const String& templ, const StringMap& properties);
  }
}

#endif //__FS_TOOLS_H_DEFINED__
