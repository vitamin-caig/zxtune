#ifndef __BACKEND_ENUMERATOR_H_DEFINED__
#define __BACKEND_ENUMERATOR_H_DEFINED__

#include "sound_backend.h"

namespace ZXTune
{
  namespace Sound
  {
    typedef Backend::Ptr (*BackendFactoryFunc)();
    typedef void (*BackendInfoFunc)(Backend::Info&);

    class BackendEnumerator
    {
    public:
      virtual ~BackendEnumerator()
      {
      }

      virtual void RegisterBackend(BackendFactoryFunc create, BackendInfoFunc describe) = 0;
      virtual void EnumerateBackends(std::vector<Backend::Info>& infos) const = 0;

      virtual Backend::Ptr CreateBackend(const String& key) const = 0;

      static BackendEnumerator& Instance();
    };

    struct BackendAutoRegistrator
    {
      BackendAutoRegistrator(BackendFactoryFunc create, BackendInfoFunc describe)
      {
        BackendEnumerator::Instance().RegisterBackend(create, describe);
      }
    };
  }
}

#endif //__BACKEND_ENUMERATOR_H_DEFINED__
