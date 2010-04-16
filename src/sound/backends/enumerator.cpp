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

#include <sound/text/sound.h>

#define FILE_TAG AE42FD5E

namespace
{
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Enumerator");

  class BackendsEnumeratorImpl : public BackendsEnumerator
  {
    typedef std::map<String, CreateBackendFunc> CreatorsStorage;
  public:
    BackendsEnumeratorImpl()
    {
      RegisterBackends(*this);
    }

    virtual void RegisterBackend(const BackendInformation& info, const CreateBackendFunc& creator)
    {
      assert(creator);
      assert(Creators.end() == Creators.find(info.Id) || !"Duplicated backend found");
      Backends.push_back(info);
      Creators.insert(CreatorsStorage::value_type(info.Id, creator));
      Log::Debug(THIS_MODULE, "Registered backend '%1%'", info.Id);
    }

    virtual void EnumerateBackends(BackendInformationArray& infos) const
    {
      infos = Backends;
    }

    virtual Error CreateBackend(const String& id, const Parameters::Map& params, Backend::Ptr& result) const
    {
      Log::Debug(THIS_MODULE, "Creating backend '%1%'", id);
      const CreatorsStorage::const_iterator it = Creators.find(id);
      if (Creators.end() == it)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_NOT_FOUND, Text::SOUND_ERROR_BACKEND_NOT_FOUND, id);
      }
      try
      {
        result = (it->second)(params);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE, Text::SOUND_ERROR_BACKEND_FAILED, id).AddSuberror(e);
      }
    }
  private:
    BackendInformationArray Backends;
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

    Error CreateBackend(const String& id, const Parameters::Map& params, Backend::Ptr& result)
    {
      try
      {
        return BackendsEnumerator::Instance().CreateBackend(id, params, result);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
    }
  }
}
