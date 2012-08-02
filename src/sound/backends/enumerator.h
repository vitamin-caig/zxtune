/*
Abstract:
  Sound backends enumerator interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __SOUND_BACKENDS_ENUMERATOR_H_DEFINED__
#define __SOUND_BACKENDS_ENUMERATOR_H_DEFINED__

//library includes
#include <sound/backend.h>

namespace ZXTune
{
  namespace Sound
  {
    class BackendsEnumerator
    {
    public:
      virtual ~BackendsEnumerator() {}

      virtual void RegisterCreator(BackendCreator::Ptr creator) = 0;
      virtual BackendCreator::Iterator::Ptr Enumerate() const = 0;

      static BackendsEnumerator& Instance();
    };

    BackendCreator::Ptr CreateDisabledBackendStub(const String& id, const String& description, uint_t caps);
    BackendCreator::Ptr CreateUnavailableBackendStub(const String& id, const String& description, uint_t caps, const Error& status);
  }
}

#endif //__SOUND_BACKENDS_ENUMERATOR_H_DEFINED__
