#ifndef __ARCHIVE_H_DEFINED__
#define __ARCHIVE_H_DEFINED__

namespace ZXTune
{
  namespace Archive
  {
    bool Check(const void* data, std::size_t size, String& depacker);
    bool Depack(const void* data, std::size_t size, Dump& target, String& depacker);
    void GetDepackersList(StringArray& depackers);
  }
}

#endif //__ARCHIVE_H_DEFINED__
