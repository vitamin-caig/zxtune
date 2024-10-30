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
#include <strings/array.h>
#include <strings/split.h>
#include <tools/locale_helpers.h>

namespace Sound
{
  const Debug::Stream Dbg("Sound::Backend");

  class StaticBackendInformation : public BackendInformation
  {
  public:
    StaticBackendInformation(BackendId id, const char* descr, uint_t caps, Error status)
      : IdValue(id)
      , DescrValue(descr)
      , CapsValue(caps)
      , StatusValue(std::move(status))
    {}

    BackendId Id() const override
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
    const BackendId IdValue;
    const char* const DescrValue;
    const uint_t CapsValue;
    const Error StatusValue;
  };

  class ServiceImpl
    : public Service
    , public BackendsStorage
  {
  public:
    using RWPtr = std::shared_ptr<ServiceImpl>;

    explicit ServiceImpl(Parameters::Accessor::Ptr options)
      : Options(std::move(options))
    {}

    std::span<const BackendInformation::Ptr> EnumerateBackends() const override
    {
      return {Infos};
    }

    std::vector<BackendId> GetAvailableBackends() const override
    {
      auto available = GetAvailable();
      std::vector<BackendId> result;
      for (const auto& id : GetOrder())
      {
        const auto avIt = std::find(available.begin(), available.end(), id);
        if (avIt != available.end())
        {
          result.push_back(*avIt);
          available.erase(avIt);
        }
      }
      std::copy(available.begin(), available.end(), std::back_inserter(result));
      return result;
    }

    Backend::Ptr CreateBackend(BackendId backendId, Module::Holder::Ptr module,
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

    void Register(BackendId id, const char* description, uint_t caps, BackendWorkerFactory::Ptr factory) override
    {
      Factories.emplace_back(id, std::move(factory));
      Infos.emplace_back(MakePtr<StaticBackendInformation>(id, description, caps, Error()));
      Dbg("Service({}): Registered backend {}", static_cast<void*>(this), id);
    }

    void Register(BackendId id, const char* description, uint_t caps, const Error& status) override
    {
      Infos.emplace_back(MakePtr<StaticBackendInformation>(id, description, caps, status));
      Dbg("Service({}): Registered disabled backend {}", static_cast<void*>(this), id);
    }

    void Register(BackendId id, const char* description, uint_t caps) override
    {
      auto status = Error(THIS_LINE, translate("Not supported in current configuration"));
      Infos.emplace_back(MakePtr<StaticBackendInformation>(id, description, caps, std::move(status)));
      Dbg("Service({}): Registered stub backend {}", static_cast<void*>(this), id);
    }

  private:
    Strings::Array GetOrder() const
    {
      const auto order = Parameters::GetString(*Options, Parameters::ZXTune::Sound::Backends::ORDER);
      const auto& elements = Strings::Split(order, [](auto c) { return !IsAlNum(c); });
      return {elements.begin(), elements.end()};
    }

    std::vector<BackendId> GetAvailable() const
    {
      std::vector<BackendId> ids;
      for (const auto& info : Infos)
      {
        if (!info->Status())
        {
          ids.push_back(info->Id());
        }
      }
      return ids;
    }

    BackendWorkerFactory::Ptr FindFactory(BackendId id) const
    {
      const auto it = std::find(Factories.begin(), Factories.end(), id);
      return it != Factories.end() ? it->Factory : BackendWorkerFactory::Ptr();
    }

  private:
    const Parameters::Accessor::Ptr Options;
    std::vector<BackendInformation::Ptr> Infos;
    struct FactoryWithId
    {
      BackendId Id;
      BackendWorkerFactory::Ptr Factory;

      FactoryWithId(BackendId id, BackendWorkerFactory::Ptr factory)
        : Id(id)
        , Factory(std::move(factory))
      {}

      bool operator==(BackendId id) const
      {
        return Id == id;
      }
    };
    std::vector<FactoryWithId> Factories;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options)
  {
    auto result = MakeRWPtr<ServiceImpl>(std::move(options));
    RegisterSystemBackends(*result);
    return result;
  }

  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options)
  {
    auto result = MakeRWPtr<ServiceImpl>(std::move(options));
    RegisterFileBackends(*result);
    return result;
  }

  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options)
  {
    auto result = MakeRWPtr<ServiceImpl>(std::move(options));
    RegisterAllBackends(*result);
    return result;
  }
}  // namespace Sound
