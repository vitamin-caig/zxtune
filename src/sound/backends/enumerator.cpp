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

  inline bool BackendInfoComparator(const BackendInfo& lh, const BackendInfo& rh)
  {
    return lh.Id < rh.Id;
  }

  class BackendsEnumeratorImpl : public BackendsEnumerator
  {
    typedef std::map<BackendInfo, CreateBackendFunc,
      bool(*)(const BackendInfo& lh, const BackendInfo& rh)> BackendsStorage;
  public:
    BackendsEnumeratorImpl()
      : Backends(BackendsStorage(BackendInfoComparator))
    {
      RegisterBackends(*this);
    }

    virtual void RegisterBackend(const BackendInfo& info, const CreateBackendFunc& creator)
    {
      assert(creator);
      assert(Backends.end() == Backends.find(info) || !"Duplicated backend found");
      Backends.insert(BackendsStorage::value_type(info, creator));
      Log::Debug(THIS_MODULE, "Registered backend '%1%'", info.Id);
    }

    virtual void EnumerateBackends(BackendInfoArray& infos) const
    {
      infos.resize(Backends.size());
      std::transform(Backends.begin(), Backends.end(), infos.begin(),
        boost::bind(&BackendsStorage::value_type::first, _1));
    }

    virtual Error CreateBackend(const String& id, Backend::Ptr& result) const
    {
      Log::Debug(THIS_MODULE, "Creating backend '%1%'", id);
      const BackendInfo key = {id, String(), String()};
      const BackendsStorage::const_iterator it(Backends.find(key));
      if (Backends.end() == it)
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

    void EnumerateBackends(BackendInfoArray& infos)
    {
      return BackendsEnumerator::Instance().EnumerateBackends(infos);
    }

    Error CreateBackend(const String& id, Backend::Ptr& result)
    {
      return BackendsEnumerator::Instance().CreateBackend(id, result);
    }
  }
}
