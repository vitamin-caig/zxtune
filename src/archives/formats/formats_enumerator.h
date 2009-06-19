#ifndef __FORMATS_ENUMERATOR_H_DEFINED__
#define __FORMATS_ENUMERATOR_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Archive
  {
    typedef bool (*CheckDumpFunc)(const void*, std::size_t);
    typedef bool (*DepackDumpFunc)(const void*, std::size_t, Dump& dst);

    class FormatsEnumerator
    {
    public:
      virtual ~FormatsEnumerator()
      {
      }

      virtual void RegisterFormat(const String& id, CheckDumpFunc checker, DepackDumpFunc depacker) = 0;
      virtual void GetRegisteredFormats(StringArray& formats) const = 0;
      virtual bool CheckDump(const void* data, std::size_t size, String& id) const = 0;
      virtual bool DepackDump(const void* data, std::size_t size, Dump& dst, String& id) const = 0;

      static FormatsEnumerator& Instance();
    };

    struct AutoFormatRegistrator
    {
      AutoFormatRegistrator(const String& id, CheckDumpFunc checker, DepackDumpFunc depacker)
      {
        FormatsEnumerator::Instance().RegisterFormat(id, checker, depacker);
      }
    };
  }
}

#endif //__FORMATS_ENUMERATOR_H_DEFINED__
