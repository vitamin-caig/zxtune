#ifndef __HRIP_H_DEFINED__
#define __HRIP_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Archive
  {
    enum HripResult
    {
      OK,
      INVALID,
      CORRUPTED
    };

    struct HripFile
    {
      HripFile() : Filename(), Size()
      {
      }
      HripFile(const String& name, std::size_t size) : Filename(name), Size(size)
      {
      }
      String Filename;
      std::size_t Size;
    };
    typedef std::vector<HripFile> HripFiles;
    HripResult CheckHrip(const void* data, std::size_t size, HripFiles& files);
    HripResult DepackHrip(const void* data, std::size_t size, const String& file, Dump& dst);
  }
}


#endif //__HRIP_H_DEFINED__
