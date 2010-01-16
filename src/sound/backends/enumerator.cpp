/*
Abstract:
  Sound backends enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#include "enumerator.h"
#include "backends_list.h"

#include <logging.h>
#include <error_tools.h>
#include <sound/error_codes.h>

#include <cassert>
#include <map>

#include <boost/bind.hpp>

#include <text/sound.h>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Enumerator");

  class BackendsEnumeratorImpl : public BackendsEnumerator
  {
    typedef std::multimap<unsigned, BackendInformation> BackendsStorage;
    typedef std::map<String, CreateBackendFunc> CreatorsStorage;
  public:
    BackendsEnumeratorImpl()
    {
      RegisterBackends(*this);
    }

    virtual void RegisterBackend(const BackendInformation& info, const CreateBackendFunc& creator, unsigned priority)
    {
      assert(creator);
      assert(Creators.end() == Creators.find(info.Id) || !"Duplicated backend found");
      Backends.insert(BackendsStorage::value_type(priority, info));
      Creators.insert(CreatorsStorage::value_type(info.Id, creator));
      Log::Debug(THIS_MODULE, "Registered backend '%1%'", info.Id);
    }

    virtual void EnumerateBackends(BackendInformationArray& infos) const
    {
      infos.resize(Backends.size());
      std::transform(Backends.begin(), Backends.end(), infos.begin(),
        boost::mem_fn(&BackendsStorage::value_type::second));
    }

    virtual Error CreateBackend(const String& id, Backend::Ptr& result) const
    {
      Log::Debug(THIS_MODULE, "Creating backend '%1%'", id);
      const CreatorsStorage::const_iterator it(Creators.find(id));
      if (Creators.end() == it)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_NOT_FOUND, TEXT_SOUND_ERROR_BACKEND_NOT_FOUND, id);
      }
      try
      {
        result = (it->second)();
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE, TEXT_SOUND_ERROR_BACKEND_FAILED, id).AddSuberror(e);
      }
    }
  private:
    BackendsStorage Backends;
    CreatorsStorage Creators;
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

    void EnumerateBackends(BackendInformationArray& infos)
    {
      return BackendsEnumerator::Instance().EnumerateBackends(infos);
    }

    Error CreateBackend(const String& id, Backend::Ptr& result)
    {
      return BackendsEnumerator::Instance().CreateBackend(id, result);
    }
  }
}
