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
//text includes
#include <sound/text/sound.h>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Enumerator");

  typedef std::list<BackendCreator::Ptr> BackendCreatorsList;

  class BackendCreatorIteratorImpl : public BackendCreator::Iterator
  {
    class BackendCreatorStub : public BackendCreator
    {
    public:
      virtual String Id() const
      {
        return String();
      }

      virtual String Description() const
      {
        return String();
      }

      virtual String Version() const
      {
        return String();
      }

      virtual uint_t Capabilities() const
      {
        return 0;
      }

      virtual Error CreateBackend(Parameters::Accessor::Ptr /*params*/, Module::Holder::Ptr /*module*/, Backend::Ptr& /*result*/) const
      {
        return Error(THIS_LINE, BACKEND_NOT_FOUND, Text::SOUND_ERROR_BACKEND_NOT_FOUND);
      }

      static void Deleter(BackendCreatorStub*)
      {
      }
    };
  public:
    BackendCreatorIteratorImpl(BackendCreatorsList::const_iterator from,
                               BackendCreatorsList::const_iterator to)
      : Pos(from), Limit(to)
    {
    }

    virtual bool IsValid() const
    {
      return Pos != Limit;
    }

    virtual BackendCreator::Ptr Get() const
    {
      //since this implementation is passed to external client, make it as safe as possible
      if (Pos != Limit)
      {
        return *Pos;
      }
      assert(!"BackendCreator iterator is out of range");
      static BackendCreatorStub stub;
      return BackendCreator::Ptr(&stub, &BackendCreatorStub::Deleter);
    }

    virtual void Next()
    {
      if (Pos != Limit)
      {
        ++Pos;
      }
      else
      {
        assert(!"BackendCreator iterator is out of range");
      }
    }
  private:
    BackendCreatorsList::const_iterator Pos;
    const BackendCreatorsList::const_iterator Limit;
  };

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
      return BackendCreator::Iterator::Ptr(new BackendCreatorIteratorImpl(Creators.begin(), Creators.end()));
    }
  private:
    BackendCreatorsList Creators;
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
  }
}
