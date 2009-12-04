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
#include "../error_codes.h"

#include <map>
#include <cassert>

#include <boost/bind.hpp>

#include <text/sound.h>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune::Sound;

  inline bool BackendInfoComparator(const Backend::Info& lh, const Backend::Info& rh)
  {
    return lh.Id < rh.Id;
  }

  class BackendsEnumeratorImpl : public BackendsEnumerator
  {
    typedef std::map<Backend::Info, CreateBackendFunc,
      bool(*)(const Backend::Info& lh, const Backend::Info& rh)> BackendsStorage;
  public:
    BackendsEnumeratorImpl()
      : Backends(BackendsStorage(BackendInfoComparator))
    {
      RegisterBackends(*this);
    }

    virtual void RegisterBackend(const Backend::Info& info, const CreateBackendFunc& creator)
    {
      assert(creator);
      const bool inserted = Backends.insert(BackendsStorage::value_type(info, creator)).second;
      assert(inserted);
    }

    virtual void EnumerateBackends(std::vector<Backend::Info>& infos) const
    {
      infos.resize(Backends.size());
      std::transform(Backends.begin(), Backends.end(), infos.begin(),
        boost::bind(&BackendsStorage::value_type::first, _1));
    }

    virtual Error CreateBackend(const String& id, Backend::Ptr& result) const
    {
      const Backend::Info key = {id, String(), String()};
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

    void EnumerateBackends(std::vector<Backend::Info>& infos)
    {
      return BackendsEnumerator::Instance().EnumerateBackends(infos);
    }

    Error CreateBackend(const String& id, Backend::Ptr& result)
    {
      return BackendsEnumerator::Instance().CreateBackend(id, result);
    }
  }
}
