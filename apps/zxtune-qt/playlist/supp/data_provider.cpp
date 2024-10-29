/**
 *
 * @file
 *
 * @brief Playlist data provider implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "data_provider.h"
#include "playlist/parameters.h"
#include "supp/thread_utils.h"
#include "ui/format.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <core/additional_files_resolve.h>
#include <core/data_location.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/service.h>
#include <debug/log.h>
#include <io/api.h>
#include <module/attributes.h>
#include <module/properties/path.h>
#include <parameters/merged_accessor.h>
#include <parameters/merged_container.h>
#include <parameters/template.h>
#include <parameters/tracking.h>
#include <strings/encoding.h>
#include <strings/format.h>
#include <strings/template.h>
#include <tools/progress_callback.h>
// std includes
#include <deque>
#include <mutex>

namespace
{
  const Debug::Stream Dbg("Playlist::DataProvider");
}

namespace
{
  void EnsureNotMainThread()
  {
    Require(!MainThread::IsCurrent());
  }

  class DataProvider
  {
  public:
    using Ptr = std::shared_ptr<const DataProvider>;

    virtual ~DataProvider() = default;

    virtual Binary::Container::Ptr GetData(StringView dataPath) const = 0;
  };

  class SimpleDataProvider : public DataProvider
  {
  public:
    explicit SimpleDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(std::move(ioParams))
    {}

    Binary::Container::Ptr GetData(StringView dataPath) const override
    {
      return IO::OpenData(dataPath, *Params, Log::ProgressCallback::Stub());
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  DataProvider::Ptr CreateSimpleDataProvider(Parameters::Accessor::Ptr ioParams)
  {
    return MakePtr<SimpleDataProvider>(std::move(ioParams));
  }

  template<class T>
  struct ObjectTraits;

  template<>
  struct ObjectTraits<Binary::Container::Ptr>
  {
    using WeightType = std::size_t;

    static WeightType Weight(const Binary::Container::Ptr& obj)
    {
      return obj->Size();
    }
  };

  template<class T, class W = typename ObjectTraits<T>::WeightType>
  class ObjectsCache
  {
    struct Item
    {
      String Id;
      T Value;
      W Weight;

      Item()
        : Value()
        , Weight()
      {}

      Item(StringView id, T val)
        : Id(id)
        , Value(val)
        , Weight(ObjectTraits<T>::Weight(val))
      {}
    };

    using ItemsList = std::deque<Item>;

  public:
    ObjectsCache()
      : TotalWeight()
    {}

    T Find(StringView id)
    {
      if (Item* res = FindItem(id))
      {
        return res->Value;
      }
      return T();
    }

    void Add(StringView id, T val)
    {
      if (Item* res = FindItem(id))
      {
        const W weight = ObjectTraits<T>::Weight(val);
        TotalWeight = TotalWeight + weight - res->Weight;
        res->Value = val;
        res->Weight = weight;
      }
      else
      {
        const Item item(id, val);
        Items.push_front(item);
        TotalWeight += item.Weight;
      }
    }

    void Del(StringView id)
    {
      for (auto it = Items.begin(), lim = Items.end(); it != lim; ++it)
      {
        if (it->Id == id)
        {
          TotalWeight -= ObjectTraits<T>::Weight(it->Value);
          Items.erase(it);
          break;
        }
      }
    }

    void Fit(std::size_t maxCount, W maxWeight)
    {
      while (Items.size() > maxCount || TotalWeight > maxWeight)
      {
        const Item entry = Items.back();
        Items.pop_back();
        TotalWeight -= entry.Weight;
      }
    }

    void Clear()
    {
      ItemsList().swap(Items);
      TotalWeight = 0;
    }

    std::size_t GetItemsCount() const
    {
      return Items.size();
    }

    W GetItemsWeight() const
    {
      return TotalWeight;
    }

  private:
    Item* FindItem(StringView id)
    {
      for (auto it = Items.begin(), lim = Items.end(); it != lim; ++it)
      {
        if (it->Id == id)
        {
          if (it != Items.begin() && Items.size() > 1)
          {
            std::iter_swap(it, Items.begin());
          }
          return &Items.front();
        }
      }
      return nullptr;
    }

  private:
    ItemsList Items;
    W TotalWeight;
  };

  class CacheParameters
  {
  public:
    explicit CacheParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    std::size_t MemoryLimit() const
    {
      using namespace Parameters::ZXTuneQT::Playlist::Cache;
      const auto mb = Parameters::GetInteger(*Params, MEMORY_LIMIT_MB, MEMORY_LIMIT_MB_DEFAULT);
      return static_cast<std::size_t>(mb * 1048576);
    }

    std::size_t FilesLimit() const
    {
      using namespace Parameters::ZXTuneQT::Playlist::Cache;
      return Parameters::GetInteger<std::size_t>(*Params, FILES_LIMIT, FILES_LIMIT_DEFAULT);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  // cached data provider
  class CachedDataProvider : public DataProvider
  {
  public:
    using Ptr = std::shared_ptr<CachedDataProvider>;

    explicit CachedDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(ioParams)
      , Delegate(CreateSimpleDataProvider(std::move(ioParams)))
    {}

    Binary::Container::Ptr GetData(StringView dataPath) const override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      const std::size_t filesLimit = Params.FilesLimit();
      const std::size_t memLimit = Params.MemoryLimit();
      if (filesLimit != 0 && memLimit != 0)
      {
        return GetCachedData(dataPath, filesLimit, memLimit);
      }
      else
      {
        Cache.Clear();
        return Delegate->GetData(dataPath);
      }
    }

    void FlushCachedData(StringView dataPath)
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      if (Cache.GetItemsCount())
      {
        Cache.Del(dataPath);
        ReportCache();
      }
    }

  private:
    Binary::Container::Ptr GetCachedData(StringView dataPath, std::size_t filesLimit, std::size_t memLimit) const
    {
      if (auto cached = Cache.Find(dataPath))
      {
        return cached;
      }
      auto data = Delegate->GetData(dataPath);
      Cache.Add(dataPath, data);
      Cache.Fit(filesLimit, memLimit);
      ReportCache();
      return data;
    }

    void ReportCache() const
    {
      Dbg("Cache({}): {} files, {} bytes", static_cast<const void*>(this), Cache.GetItemsCount(),
          Cache.GetItemsWeight());
    }

  private:
    const CacheParameters Params;
    const DataProvider::Ptr Delegate;
    mutable std::mutex Mutex;
    mutable ObjectsCache<Binary::Container::Ptr> Cache;
  };

  class DataSource : public Module::AdditionalFilesSource
  {
  public:
    using Ptr = std::shared_ptr<const DataSource>;

    DataSource(CachedDataProvider::Ptr provider, IO::Identifier::Ptr id)
      : Provider(std::move(provider))
      , DataId(std::move(id))
      , Dir(ExtractDir(*DataId))
    {}

    ~DataSource() override
    {
      Provider->FlushCachedData(DataId->Path());
    }

    Binary::Container::Ptr GetData() const
    {
      return Provider->GetData(DataId->Path());
    }

    IO::Identifier::Ptr GetDataIdentifier() const
    {
      return DataId;
    }

    // AdditionalFilesSource
    Binary::Container::Ptr Get(StringView name) const override
    {
      return Provider->GetData(Dir + name);
    }

  private:
    static String ExtractDir(const IO::Identifier& id)
    {
      const auto& full = id.Full();
      const auto& filename = id.Filename();
      Require(!filename.empty());
      return full.substr(0, full.size() - filename.size());
    }

  private:
    const CachedDataProvider::Ptr Provider;
    const IO::Identifier::Ptr DataId;
    const String Dir;
  };

  class ModuleSource
  {
  public:
    ModuleSource(ZXTune::Service::Ptr service, DataSource::Ptr source, IO::Identifier::Ptr moduleId)
      : Service(std::move(service))
      , Source(std::move(source))
      , ModuleId(std::move(moduleId))
    {}

    Module::Holder::Ptr GetModule(Parameters::Accessor::Ptr adjustedParams) const
    {
      auto forcedProperties =
          Parameters::CreateMergedAccessor(Module::CreatePathProperties(ModuleId), std::move(adjustedParams));
      // All the parsed data is written to new container, but adjustedParam is shadowing it
      auto initialProperties =
          Parameters::CreateMergedContainer(std::move(forcedProperties), Parameters::Container::Create());
      auto data = Source->GetData();
      const auto& subpath = ModuleId->Subpath();
      auto module = Service->OpenModule(std::move(data), subpath, std::move(initialProperties));
      if (subpath.empty())
      {
        if (const auto* const files = dynamic_cast<const Module::AdditionalFiles*>(module.get()))
        {
          Module::ResolveAdditionalFiles(*Source, *files);
        }
      }
      return module;
    }

    Binary::Data::Ptr GetModuleData(std::size_t size) const
    {
      auto data = Source->GetData();
      return Service->OpenData(std::move(data), ModuleId->Subpath())->GetSubcontainer(0, size);
    }

    String GetFullPath() const
    {
      return ModuleId->Full();
    }

    String GetFilePath() const
    {
      return ModuleId->Path();
    }

  private:
    const ZXTune::Service::Ptr Service;
    const DataSource::Ptr Source;
    const IO::Identifier::Ptr ModuleId;
  };

  class DynamicAttributesProvider
  {
  public:
    using Ptr = std::shared_ptr<const DynamicAttributesProvider>;

    DynamicAttributesProvider()
      : DisplayNameTemplate(Strings::Template::Create("[Author] - [Title]"))
      , DummyDisplayName(DisplayNameTemplate->Instantiate(Strings::SkipFieldsSource()))
    {}

    String GetDisplayName(const Parameters::Accessor& properties) const
    {
      const Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource> adapter(properties);
      String result = DisplayNameTemplate->Instantiate(adapter);
      if (result == DummyDisplayName)
      {
        result = Parameters::GetString(properties, Module::ATTR_FULLPATH);
      }
      return result;
    }

  private:
    const Strings::Template::Ptr DisplayNameTemplate;
    const String DummyDisplayName;
  };

  class DataImpl : public Playlist::Item::Data
  {
  public:
    using Ptr = Playlist::Item::Data::Ptr;

    DataImpl(DynamicAttributesProvider::Ptr attributes, ModuleSource source, Parameters::Accessor::Ptr moduleProps,
             Parameters::Container::Ptr adjustedParams, Time::Milliseconds duration, uint_t caps)
      : Caps(caps)
      , Attributes(std::move(attributes))
      , Source(std::move(source))
      , AdjustedParams(std::move(adjustedParams))
      , Properties(Parameters::CreateMergedAccessor(AdjustedParams, std::move(moduleProps)))
      , Duration(duration)
      , CurrentState(Playlist::Item::ModuleState::MakeReady())
    {}

    Module::Holder::Ptr GetModule() const override
    {
      try
      {
        CurrentState = Playlist::Item::ModuleState::MakeLoading();
        auto res = Source.GetModule(AdjustedParams);
        CurrentState = Playlist::Item::ModuleState::MakeReady();
        return res;
      }
      catch (const Error& e)
      {
        CurrentState = Playlist::Item::ModuleState::MakeReady(e);
      }
      return {};
    }

    Binary::Data::Ptr GetModuleData() const override
    {
      return Source.GetModuleData(GetSize());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Parameters::Container::Ptr GetAdjustedParameters() const override
    {
      return AdjustedParams;
    }

    Playlist::Item::Capabilities GetCapabilities() const override
    {
      return Caps;
    }

    // playlist-related properties
    Playlist::Item::ModuleState GetState() const override
    {
      return CurrentState;
    }

    String GetFullPath() const override
    {
      return Source.GetFullPath();
    }

    String GetFilePath() const override
    {
      return Source.GetFilePath();
    }

    String GetType() const override
    {
      return Parameters::GetString(*Properties, Module::ATTR_TYPE);
    }

    String GetDisplayName() const override
    {
      return Attributes->GetDisplayName(*Properties);
    }

    Time::Milliseconds GetDuration() const override
    {
      return Duration;
    }

    String GetAuthor() const override
    {
      return Parameters::GetString(*Properties, Module::ATTR_AUTHOR);
    }

    String GetTitle() const override
    {
      return Parameters::GetString(*Properties, Module::ATTR_TITLE);
    }

    String GetComment() const override
    {
      return Parameters::GetString(*Properties, Module::ATTR_COMMENT);
    }

    uint32_t GetChecksum() const override
    {
      return Parameters::GetInteger<uint32_t>(*Properties, Module::ATTR_CRC);
    }

    uint32_t GetCoreChecksum() const override
    {
      return Parameters::GetInteger<uint32_t>(*Properties, Module::ATTR_FIXEDCRC);
    }

    std::size_t GetSize() const override
    {
      return Parameters::GetInteger<std::size_t>(*Properties, Module::ATTR_SIZE);
    }

  private:
    const Playlist::Item::Capabilities Caps;
    const DynamicAttributesProvider::Ptr Attributes;
    const ModuleSource Source;
    const Parameters::Container::Ptr AdjustedParams;
    const Parameters::Accessor::Ptr Properties;
    Time::Milliseconds Duration;
    mutable Playlist::Item::ModuleState CurrentState;
  };

  class DetectCallback : public Module::DetectCallback
  {
  public:
    DetectCallback(Playlist::Item::DetectParameters& delegate, DynamicAttributesProvider::Ptr attributes,
                   CachedDataProvider::Ptr provider, ZXTune::Service::Ptr service, IO::Identifier::Ptr dataId)
      : Delegate(delegate)
      , Attributes(std::move(attributes))
      , Service(std::move(service))
      , DataId(dataId)
      , Source(MakePtr<DataSource>(std::move(provider), std::move(dataId)))
    {}

    Parameters::Container::Ptr CreateInitialProperties(StringView subpath) const override
    {
      auto moduleId = DataId->WithSubpath(subpath);
      auto pathProps = Module::CreatePathProperties(std::move(moduleId));
      return Parameters::CreateMergedContainer(std::move(pathProps), Parameters::Container::Create());
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder,
                       Module::Holder::Ptr holder) override
    {
      const String subPath = location.GetPath()->AsString();
      if (subPath.empty())
      {
        if (const auto* const files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
        {
          Module::ResolveAdditionalFiles(*Source, *files);
        }
      }
      const auto info = holder->GetModuleInformation();
      ModuleSource itemSource(Service, Source, DataId->WithSubpath(subPath));
      auto playitem = MakePtr<DataImpl>(Attributes, std::move(itemSource), holder->GetModuleProperties(),
                                        Delegate.CreateInitialAdjustedParameters(), info->Duration(),
                                        decoder.Capabilities());
      Delegate.ProcessItem(std::move(playitem));
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }

  private:
    Playlist::Item::DetectParameters& Delegate;
    const DynamicAttributesProvider::Ptr Attributes;
    const ZXTune::Service::Ptr Service;
    const IO::Identifier::Ptr DataId;
    const DataSource::Ptr Source;
  };

  class ThreadCheckingService : public ZXTune::Service
  {
  public:
    ThreadCheckingService(Parameters::Accessor::Ptr parameters)
      : Delegate(ZXTune::Service::Create(std::move(parameters)))
    {}

    Binary::Container::Ptr OpenData(Binary::Container::Ptr data, StringView subpath) const override
    {
      EnsureNotMainThread();
      return Delegate->OpenData(std::move(data), subpath);
    }

    Module::Holder::Ptr OpenModule(Binary::Container::Ptr data, StringView subpath,
                                   Parameters::Container::Ptr initialProperties) const override
    {
      EnsureNotMainThread();
      return Delegate->OpenModule(std::move(data), subpath, std::move(initialProperties));
    }

    void DetectModules(Binary::Container::Ptr data, Module::DetectCallback& callback) const override
    {
      EnsureNotMainThread();
      return Delegate->DetectModules(std::move(data), callback);
    }

    void OpenModule(Binary::Container::Ptr data, StringView subpath, Module::DetectCallback& callback) const override
    {
      EnsureNotMainThread();
      return Delegate->OpenModule(std::move(data), subpath, callback);
    }

  private:
    const Ptr Delegate;
  };

  class DataProviderImpl : public Playlist::Item::DataProvider
  {
  public:
    explicit DataProviderImpl(Parameters::Accessor::Ptr parameters)
      : Provider(MakePtr<CachedDataProvider>(parameters))
      , Service(MakePtr<ThreadCheckingService>(std::move(parameters)))
      , Attributes(MakePtr<DynamicAttributesProvider>())
    {}

    void DetectModules(StringView path, Playlist::Item::DetectParameters& detectParams) const override
    {
      auto id = IO::ResolveUri(path);

      const auto subPath = id->Subpath();
      if (subPath.empty())
      {
        auto data = Provider->GetData(id->Path());
        DetectCallback detectCallback(detectParams, Attributes, Provider, Service, std::move(id));
        Service->DetectModules(std::move(data), detectCallback);
      }
      else
      {
        OpenModule(path, detectParams);
      }
    }

    void OpenModule(StringView path, Playlist::Item::DetectParameters& detectParams) const override
    {
      auto id = IO::ResolveUri(path);

      auto data = Provider->GetData(id->Path());
      DetectCallback detectCallback(detectParams, Attributes, Provider, Service, id);
      Service->OpenModule(std::move(data), id->Subpath(), detectCallback);
    }

  private:
    const CachedDataProvider::Ptr Provider;
    const ZXTune::Service::Ptr Service;
    const DynamicAttributesProvider::Ptr Attributes;
  };
}  // namespace

namespace Playlist::Item
{
  ModuleState ModuleState::Make()
  {
    return ModuleState(Empty{});
  }

  ModuleState ModuleState::MakeLoading()
  {
    return ModuleState(Loading{});
  }

  ModuleState ModuleState::MakeReady(Error err)
  {
    return err ? ModuleState(std::move(err)) : ModuleState(Ready{});
  }

  bool ModuleState::IsLoading() const
  {
    return std::get_if<Loading>(&Container) != nullptr;
  }

  bool ModuleState::IsReady() const
  {
    return std::get_if<Ready>(&Container) != nullptr || GetIfError() != nullptr;
  }

  const Error* ModuleState::GetIfError() const
  {
    return std::get_if<Error>(&Container);
  }

  DataProvider::Ptr DataProvider::Create(Parameters::Accessor::Ptr parameters)
  {
    return MakePtr<DataProviderImpl>(std::move(parameters));
  }
}  // namespace Playlist::Item
