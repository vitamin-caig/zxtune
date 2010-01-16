/*
Abstract:
  Sound backends enumerator interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_BACKENDS_ENUMERATOR_H_DEFINED__
#define __SOUND_BACKENDS_ENUMERATOR_H_DEFINED__

#include <sound/backend.h>

#include <boost/function.hpp>

namespace ZXTune
{
  namespace Sound
  {
    typedef boost::function<Backend::Ptr()> CreateBackendFunc;

    enum
    {
      BACKEND_PRIORITY_HIGH = 0,
      BACKEND_PRIORITY_MID = 50,
      BACKEND_PRIORITY_LOW = 100,
    };
    
    class BackendsEnumerator
    {
    public:
      virtual ~BackendsEnumerator()
      {
      }

      virtual void RegisterBackend(const BackendInformation& info, const CreateBackendFunc& creator, unsigned priority) = 0;
      virtual void EnumerateBackends(BackendInformationArray& infos) const = 0;

      virtual Error CreateBackend(const String& id, Backend::Ptr& result) const = 0;

      static BackendsEnumerator& Instance();
    };
  }
}

#endif //__SOUND_BACKENDS_ENUMERATOR_H_DEFINED__
