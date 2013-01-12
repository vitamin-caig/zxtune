/*
Abstract:
  Sound backends enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
#include "backends_list.h"
//common includes
#include <error_tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
//std includes
#include <cassert>
#include <list>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Enumerator");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");

  typedef std::list<BackendCreator::Ptr> BackendCreatorsList;

  class BackendsEnumeratorImpl : public BackendsEnumerator
  {
  public:
    BackendsEnumeratorImpl()
    {
      RegisterBackends(*this);
    }

    virtual void RegisterCreator(BackendCreator::Ptr creator)
    {
      assert(creator);
      Creators.push_back(creator);
      Dbg("Registered backend '%1%'", creator->Id());
    }

    virtual BackendCreator::Iterator::Ptr Enumerate() const
    {
      return BackendCreator::Iterator::Ptr(new RangedObjectIteratorAdapter<BackendCreatorsList::const_iterator, BackendCreator::Ptr>(
        Creators.begin(), Creators.end()));
    }
  private:
    BackendCreatorsList Creators;
  };

  class UnavailableBackend : public BackendCreator
  {
  public:
    UnavailableBackend(const String& id, const char* descr, uint_t caps, const Error& status)
      : IdValue(id)
      , DescrValue(descr)
      , CapsValue(caps)
      , StatusValue(status)
    {
    }

    virtual String Id() const
    {
      return IdValue;
    }

    virtual String Description() const
    {
      return translate(DescrValue);
    }

    virtual uint_t Capabilities() const
    {
      return CapsValue;
    }

    virtual Error Status() const
    {
      return StatusValue;
    }

    virtual Backend::Ptr CreateBackend(CreateBackendParameters::Ptr) const
    {
      throw Error(THIS_LINE, translate("Requested backend is not supported."));
    }
  private:
    const String IdValue;
    const char* const DescrValue;
    const uint_t CapsValue;
    const Error StatusValue;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    BackendsEnumerator& BackendsEnumerator::Instance()
    {
      static BackendsEnumeratorImpl instance;
      return instance;
    }

    BackendCreator::Iterator::Ptr EnumerateBackends()
    {
      return BackendsEnumerator::Instance().Enumerate();
    }

    BackendCreator::Ptr CreateDisabledBackendStub(const String& id, const char* description, uint_t caps)
    {
      return CreateUnavailableBackendStub(id, description, caps, Error(THIS_LINE, translate("Not supported in current configuration")));
    }

    BackendCreator::Ptr CreateUnavailableBackendStub(const String& id, const char* description, uint_t caps, const Error& status)
    {
      return boost::make_shared<UnavailableBackend>(id, description, caps, status);
    }
  }
}
