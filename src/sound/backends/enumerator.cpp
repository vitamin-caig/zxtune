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
#include <logging.h>
#include <error_tools.h>
//library includes
#include <sound/error_codes.h>
//std includes
#include <cassert>
#include <list>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/sound.h>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Enumerator");

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
      Log::Debug(THIS_MODULE, "Registered backend '%1%'", creator->Id());
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
    UnavailableBackend(const String& id, const String& descr, uint_t caps, const Error& status)
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
      return DescrValue;
    }

    virtual uint_t Capabilities() const
    {
      return CapsValue;
    }

    virtual Error Status() const
    {
      return StatusValue;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr, Backend::Ptr&) const
    {
      return Error(THIS_LINE, BACKEND_NOT_FOUND, Text::SOUND_ERROR_BACKEND_NOT_FOUND);
    }
  private:
    const String IdValue;
    const String DescrValue;
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

    BackendCreator::Ptr CreateDisabledBackendStub(const String& id, const String& description, uint_t caps)
    {
      return CreateUnavailableBackendStub(id, description, caps, Error(THIS_LINE, BACKEND_NOT_FOUND, Text::SOUND_ERROR_DISABLED_BACKEND));
    }

    BackendCreator::Ptr CreateUnavailableBackendStub(const String& id, const String& description, uint_t caps, const Error& status)
    {
      return boost::make_shared<UnavailableBackend>(id, description, caps, status);
    }
  }
}
