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

//library includes
#include <sound/backend.h>
//boost includes
#include <boost/function.hpp>

namespace ZXTune
{
  namespace Sound
  {
    typedef boost::function<Backend::Ptr(const Parameters::Map&)> CreateBackendFunc;

    class BackendsEnumerator
    {
    public:
      virtual ~BackendsEnumerator() {}

      virtual void RegisterBackend(const BackendInformation& info, const CreateBackendFunc& creator) = 0;
      virtual void EnumerateBackends(BackendInformationArray& infos) const = 0;

      virtual Error CreateBackend(const String& id, const Parameters::Map& params, Backend::Ptr& result) const = 0;

      static BackendsEnumerator& Instance();
    };
  }
}

#endif //__SOUND_BACKENDS_ENUMERATOR_H_DEFINED__
