/**
 *
 * @file
 *
 * @brief  Sound service implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/backend_impl.h"
#include "sound/backends/backends_list.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/service.h>
// boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define FILE_TAG A6428476

namespace Sound
{
  const Debug::Stream Dbg("Sound::Backend");

  class StaticBackendInformation : public BackendInformation
  {
  public:
    StaticBackendInformation(String id, const char* descr, uint_t caps, Error status)
      : IdValue(std::move(id))
      , DescrValue(descr)
      , CapsValue(caps)
      , StatusValue(std::move(status))
    {}

    String Id() const override
    {
      return IdValue;
    }

    String Description() const override
    {
      return translate(DescrValue);
    }

    uint_t Capabilities() const override
    {
      return CapsValue;
    }

    Error Status() const override
    {
      return StatusValue;
    }

  private:
    const String IdValue;
    const char* const DescrValue;
    const uint_t CapsValue;
    const Error StatusValue;
  };

  class ServiceImpl
    : public Service
    , public BackendsStorage
  {
  public:
    typedef std::shared_ptr<ServiceImpl> RWPtr;

    explicit ServiceImpl(Parameters::Accessor::Ptr options)
      : Options(std::move(options))
    {}

    BackendInformation::Iterator::Ptr EnumerateBackends() const override
    {
      return CreateRangedObjectIteratorAdapter(Infos.begin(), Infos.end());
    }

    Strings::Array GetAvailableBackends() const override
    {
      const Strings::Array order = GetOrder();
      Strings::Array available = GetAvailable();
      Strings::Array result;
      for (const auto& id : order)
      {
        const Strings::Array::iterator avIt = std::find(available.begin(), available.end(), id);
        if (avIt != available.end())
        {
          result.push_back(*avIt);
          available.erase(avIt);
        }
      }
      std::copy(available.begin(), available.end(), std::back_inserter(result));
      return result;
    }

    Backend::Ptr CreateBackend(const String& backendId, Module::Holder::Ptr module,
                               BackendCallback::Ptr callback) const override
    {
      try
      {
        if (const auto factory = FindFactory(backendId))
        {
          return Sound::CreateBackend(Options, module, std::move(callback), factory->CreateWorker(Options, module));
        }
        throw MakeFormattedError(THIS_LINE, translate("Backend '{}' not registered."), backendId);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to create backend '{}'."), backendId).AddSuberror(e);
      }
    }

    void Register(const String& id, const char* description, uint_t caps, BackendWorkerFactory::Ptr factory) override
    {
      Factories.push_back(FactoryWithId(id, factory));
      const BackendInformation::Ptr info = MakePtr<StaticBackendInformation>(id, description, caps, Error());
      Infos.push_back(info);
      Dbg("Service({}): Registered backend {}", static_cast<void*>(this), id);
    }

    void Register(const String& id, const char* description, uint_t caps, const Error& status) override
    {
      const BackendInformation::Ptr info = MakePtr<StaticBackendInformation>(id, description, caps, status);
      Infos.push_back(info);
      Dbg("Service({}): Registered disabled backend {}", static_cast<void*>(this), id);
    }

    void Register(const String& id, const char* description, uint_t caps) override
    {
      const Error status = Error(THIS_LINE, translate("Not supported in current configuration"));
      const BackendInformation::Ptr info = MakePtr<StaticBackendInformation>(id, description, caps, status);
      Infos.push_back(info);
      Dbg("Service({}): Registered stub backend {}", static_cast<void*>(this), id);
    }

  private:
    Strings::Array GetOrder() const
    {
      Parameters::StringType order;
      Options->FindValue(Parameters::ZXTune::Sound::Backends::ORDER, order);
      Strings::Array orderArray;
      boost::algorithm::split(orderArray, order, !boost::algorithm::is_alnum());
      return orderArray;
    }

    Strings::Array GetAvailable() const
    {
      Strings::Array ids;
      for (const auto& info : Infos)
      {
        if (!info->Status())
        {
          ids.push_back(info->Id());
        }
      }
      return ids;
    }

    BackendWorkerFactory::Ptr FindFactory(const String& id) const
    {
      const std::vector<FactoryWithId>::const_iterator it = std::find(Factories.begin(), Factories.end(), id);
      return it != Factories.end() ? it->Factory : BackendWorkerFactory::Ptr();
    }

  private:
    const Parameters::Accessor::Ptr Options;
    std::vector<BackendInformation::Ptr> Infos;
    struct FactoryWithId
    {
      String Id;
      BackendWorkerFactory::Ptr Factory;

      FactoryWithId() {}

      FactoryWithId(String id, BackendWorkerFactory::Ptr factory)
        : Id(std::move(id))
        , Factory(std::move(factory))
      {}

      bool operator==(const String& id) const
      {
        return Id == id;
      }
    };
    std::vector<FactoryWithId> Factories;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options)
  {
    const ServiceImpl::RWPtr result = MakeRWPtr<ServiceImpl>(options);
    RegisterSystemBackends(*result);
    return result;
  }

  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options)
  {
    const ServiceImpl::RWPtr result = MakeRWPtr<ServiceImpl>(options);
    RegisterFileBackends(*result);
    return result;
  }

  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options)
  {
    const ServiceImpl::RWPtr result = MakeRWPtr<ServiceImpl>(options);
    RegisterAllBackends(*result);
    return result;
  }
}  // namespace Sound

#undef FILE_TAG
