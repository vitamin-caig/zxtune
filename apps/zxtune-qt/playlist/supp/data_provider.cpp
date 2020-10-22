/**
* 
* @file
*
* @brief Playlist data provider implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "data_provider.h"
#include "ui/format.h"
#include "ui/utils.h"
#include "playlist/parameters.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <progress_callback.h>
//library includes
#include <core/additional_files_resolve.h>
#include <core/module_detect.h>
#include <core/module_open.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/src/location.h>
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
//std includes
#include <deque>
#include <mutex>
//text includes
#include "text/text.h"

#define FILE_TAG 0C9BBC6E

namespace
{
  const Debug::Stream Dbg("Playlist::DataProvider");
}

namespace
{
  class DataProvider
  {
  public:
    typedef std::shared_ptr<const DataProvider> Ptr;

    virtual ~DataProvider() = default;

    virtual Binary::Container::Ptr GetData(const String& dataPath) const = 0;
  };
  
  class SimpleDataProvider : public DataProvider
  {
  public:
    explicit SimpleDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(std::move(ioParams))
    {
    }

    Binary::Container::Ptr GetData(const String& dataPath) const override
    {
      return IO::OpenData(dataPath, *Params, Log::ProgressCallback::Stub());
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  DataProvider::Ptr CreateSimpleDataProvider(Parameters::Accessor::Ptr ioParams)
  {
    return MakePtr<SimpleDataProvider>(ioParams);
  }

  template<class T>
  struct ObjectTraits;

  template<>
  struct ObjectTraits<Binary::Container::Ptr>
  {
    typedef std::size_t WeightType;

    static WeightType Weight(Binary::Container::Ptr obj)
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
        : Id()
        , Value()
        , Weight()
      {
      }

      Item(String id, T val)
        : Id(std::move(id))
        , Value(val)
        , Weight(ObjectTraits<T>::Weight(val))
      {
      }
    };

    typedef std::deque<Item> ItemsList;
  public:
    ObjectsCache()
      : TotalWeight()
    {
    }

    T Find(const String& id)
    {
      if (Item* res = FindItem(id))
      {
        return res->Value;
      }
      return T();
    }

    void Add(const String& id, T val)
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

    void Del(const String& id)
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
      while (Items.size() > maxCount ||
             TotalWeight > maxWeight)
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
    Item* FindItem(const String& id)
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
    {
    }

    std::size_t MemoryLimit() const
    {
      Parameters::IntType res = Parameters::ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB, res);
      return static_cast<std::size_t>(res * 1048576);
    }

    std::size_t FilesLimit() const
    {
      Parameters::IntType res = Parameters::ZXTuneQT::Playlist::Cache::FILES_LIMIT_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::Cache::FILES_LIMIT, res);
      return static_cast<std::size_t>(res);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  //cached data provider
  class CachedDataProvider : public DataProvider
  {
  public:
    typedef std::shared_ptr<CachedDataProvider> Ptr;

    explicit CachedDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(ioParams)
      , Delegate(CreateSimpleDataProvider(ioParams))
    {
    }

    Binary::Container::Ptr GetData(const String& dataPath) const override
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

    void FlushCachedData(const String& dataPath)
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      if (Cache.GetItemsCount())
      {
        Cache.Del(dataPath);
        ReportCache();
      }
    }
  private:
    Binary::Container::Ptr GetCachedData(const String& dataPath, std::size_t filesLimit, std::size_t memLimit) const
    {
      if (const Binary::Container::Ptr cached = Cache.Find(dataPath))
      {
        return cached;
      }
      const Binary::Container::Ptr data = Delegate->GetData(dataPath);
      Cache.Add(dataPath, data);
      Cache.Fit(filesLimit, memLimit);
      ReportCache();
      return data;
    }
    
    void ReportCache() const
    {
      Dbg("Cache(%1%): %2% files, %3% bytes", this, Cache.GetItemsCount(), Cache.GetItemsWeight());
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
    typedef std::shared_ptr<const DataSource> Ptr;

    DataSource(CachedDataProvider::Ptr provider, IO::Identifier::Ptr id)
      : Provider(std::move(provider))
      , DataId(std::move(id))
      , Dir(ExtractDir(*DataId))
    {
    }

    ~DataSource()
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
    
    //AdditionalFilesSource
    Binary::Container::Ptr Get(const String& name) const override
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
    ModuleSource(Parameters::Accessor::Ptr coreParams, DataSource::Ptr source, IO::Identifier::Ptr moduleId)
      : CoreParams(std::move(coreParams))
      , Source(std::move(source))
      , ModuleId(std::move(moduleId))
    {
    }

    Module::Holder::Ptr GetModule(Parameters::Accessor::Ptr adjustedParams) const
    {
      auto forcedProperties = Parameters::CreateMergedAccessor(Module::CreatePathProperties(ModuleId), std::move(adjustedParams));
      // All the parsed data is written to new container, but adjustedParam is shadowing it
      auto initialProperties = Parameters::CreateMergedContainer(std::move(forcedProperties), Parameters::Container::Create());
      auto data = Source->GetData();
      const auto& subpath = ModuleId->Subpath();
      const auto module = Module::Open(*CoreParams, std::move(data), subpath, std::move(initialProperties));
      if (subpath.empty())
      {
        if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(module.get()))
        {
          Module::ResolveAdditionalFiles(*Source, *files);
        }
      }
      return module;
    }
    
    Binary::Data::Ptr GetModuleData(std::size_t size) const
    {
      const Binary::Container::Ptr data = Source->GetData();
      const ZXTune::DataLocation::Ptr location = ZXTune::OpenLocation(*CoreParams, data, ModuleId->Subpath());
      return location->GetData()->GetSubcontainer(0, size);
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
    const Parameters::Accessor::Ptr CoreParams;
    const DataSource::Ptr Source;
    const IO::Identifier::Ptr ModuleId;
  };

  String GetStringProperty(const Parameters::Accessor& props, const Parameters::NameType& propName)
  {
    Parameters::StringType val;
    if (props.FindValue(propName, val))
    {
      return val;
    }
    return String();
  }

  Parameters::IntType GetIntProperty(const Parameters::Accessor& props, const Parameters::NameType& propName, Parameters::IntType defVal = 0)
  {
    Parameters::IntType val = defVal;
    props.FindValue(propName, val);
    return val;
  }

  class DynamicAttributesProvider
  {
  public:
    typedef std::shared_ptr<const DynamicAttributesProvider> Ptr;

    DynamicAttributesProvider()
      : DisplayNameTemplate(Strings::Template::Create(Text::MODULE_PLAYLIST_FORMAT))
      , DummyDisplayName(DisplayNameTemplate->Instantiate(Strings::SkipFieldsSource()))
    {
    }

    String GetDisplayName(const Parameters::Accessor& properties) const
    {
      const Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource> adapter(properties);
      String result = DisplayNameTemplate->Instantiate(adapter);
      if (result == DummyDisplayName)
      {
        if (!properties.FindValue(Module::ATTR_FULLPATH, result))
        {
          result.clear();
        }
      }
      return result;
    }
  private:
    const Strings::Template::Ptr DisplayNameTemplate;
    const String DummyDisplayName;
  };

  class DataImpl : public Playlist::Item::Data
                 , private Parameters::Modifier
  {
  public:
    typedef Playlist::Item::Data::Ptr Ptr;
    
    DataImpl(DynamicAttributesProvider::Ptr attributes,
        ModuleSource source,
        Parameters::Container::Ptr adjustedParams,
        Time::Milliseconds duration, const Parameters::Accessor& moduleProps,
        uint_t caps)
      : Caps(caps)
      , Attributes(std::move(attributes))
      , Source(std::move(source))
      , AdjustedParams(std::move(adjustedParams))
      , Type(GetStringProperty(moduleProps, Module::ATTR_TYPE))
      , Checksum(static_cast<uint32_t>(GetIntProperty(moduleProps, Module::ATTR_CRC)))
      , CoreChecksum(static_cast<uint32_t>(GetIntProperty(moduleProps, Module::ATTR_FIXEDCRC)))
      , Size(static_cast<std::size_t>(GetIntProperty(moduleProps, Module::ATTR_SIZE)))
      , Duration(duration)
    {
      LoadProperties(moduleProps);
    }

    Module::Holder::Ptr GetModule() const override
    {
      try
      {
        State = Error();
        return Source.GetModule(AdjustedParams);
      }
      catch (const Error& e)
      {
        State = e;
      }
      return Module::Holder::Ptr();
    }

    Binary::Data::Ptr GetModuleData() const override
    {
      return Source.GetModuleData(Size);
    }
    
    Parameters::Container::Ptr GetAdjustedParameters() const override
    {
      const Parameters::Modifier& cb = *this;
      return Parameters::CreatePostChangePropertyTrackedContainer(AdjustedParams, const_cast<Parameters::Modifier&>(cb));
    }

    Playlist::Item::Capabilities GetCapabilities() const override
    {
      return Caps;
    }

    //playlist-related properties
    Error GetState() const override
    {
      return State;
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
      return Type;
    }

    String GetDisplayName() const override
    {
      return DisplayName;
    }

    Time::Milliseconds GetDuration() const override
    {
      return Duration;
    }

    String GetAuthor() const override
    {
      return Author;
    }

    String GetTitle() const override
    {
      return Title;
    }

    String GetComment() const override
    {
      return Comment;
    }
    
    uint32_t GetChecksum() const override
    {
      return Checksum;
    }

    uint32_t GetCoreChecksum() const override
    {
      return CoreChecksum;
    }

    std::size_t GetSize() const override
    {
      return Size;
    }
  private:
    void SetValue(const Parameters::NameType& /*name*/, Parameters::IntType /*val*/) override
    {
      OnPropertyChanged();
    }

    void SetValue(const Parameters::NameType& /*name*/, const Parameters::StringType& /*val*/) override
    {
      OnPropertyChanged();
    }

    void SetValue(const Parameters::NameType& /*name*/, const Parameters::DataType& /*val*/) override
    {
      OnPropertyChanged();
    }

    void RemoveValue(const Parameters::NameType& /*name*/) override
    {
      OnPropertyChanged();
    }

    void OnPropertyChanged()
    {
      if (const auto holder = GetModule())
      {
        LoadProperties(*holder->GetModuleProperties());
        Duration = holder->GetModuleInformation()->Duration();
      }
      else
      {
        DisplayName.clear();
        Author.clear();
        Title.clear();
        Comment.clear();
        Duration = {};
      }
    }

    void LoadProperties(const Parameters::Accessor& props)
    {
      DisplayName = Attributes->GetDisplayName(props);
      Author = GetStringProperty(props, Module::ATTR_AUTHOR);
      Title = GetStringProperty(props, Module::ATTR_TITLE);
      Comment = GetStringProperty(props, Module::ATTR_COMMENT);
    }
  private:
    const Playlist::Item::Capabilities Caps;
    const DynamicAttributesProvider::Ptr Attributes;
    const ModuleSource Source;
    const Parameters::Container::Ptr AdjustedParams;
    const String Type;
    const uint32_t Checksum;
    const uint32_t CoreChecksum;
    const std::size_t Size;
    String DisplayName;
    String Author;
    String Title;
    String Comment;
    Time::Milliseconds Duration;
    mutable Error State;
  };
  
  class DetectCallback : public Module::DetectCallback
  {
  public:
    DetectCallback(Playlist::Item::DetectParameters& delegate,
                            DynamicAttributesProvider::Ptr attributes,
                            CachedDataProvider::Ptr provider, Parameters::Accessor::Ptr coreParams, IO::Identifier::Ptr dataId)
      : Delegate(delegate)
      , Attributes(std::move(attributes))
      , CoreParams(std::move(coreParams))
      , DataId(dataId)
      , Source(MakePtr<DataSource>(provider, dataId))
    {
    }

    Parameters::Container::Ptr CreateInitialProperties(const String& subpath) const
    {
      auto moduleId = DataId->WithSubpath(subpath);
      auto pathProps = Module::CreatePathProperties(std::move(moduleId));
      return Parameters::CreateMergedContainer(std::move(pathProps), Parameters::Container::Create());
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder, Module::Holder::Ptr holder) override
    {
      const String subPath = location.GetPath()->AsString();
      if (subPath.empty())
      {
        if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
        {
          Module::ResolveAdditionalFiles(*Source, *files);
        }
      }
      auto adjustedParams = Delegate.CreateInitialAdjustedParameters();
      const auto info = holder->GetModuleInformation();
      const auto lookupModuleProps = Parameters::CreateMergedAccessor(adjustedParams, holder->GetModuleProperties());
      ModuleSource itemSource(CoreParams, Source, DataId->WithSubpath(subPath));
      auto playitem = MakePtr<DataImpl>(Attributes, std::move(itemSource), std::move(adjustedParams),
        info->Duration(), *lookupModuleProps, decoder.Capabilities());
      Delegate.ProcessItem(std::move(playitem));
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }
  private:
    Playlist::Item::DetectParameters& Delegate;
    const DynamicAttributesProvider::Ptr Attributes;
    const Parameters::Accessor::Ptr CoreParams;
    const IO::Identifier::Ptr DataId;
    const DataSource::Ptr Source;
  };

  class DataProviderImpl : public Playlist::Item::DataProvider
  {
  public:
    explicit DataProviderImpl(Parameters::Accessor::Ptr parameters)
      : Provider(MakePtr<CachedDataProvider>(parameters))
      , CoreParams(parameters)
      , Attributes(MakePtr<DynamicAttributesProvider>())
    {
    }

    void DetectModules(const String& path, Playlist::Item::DetectParameters& detectParams) const override
    {
      auto id = IO::ResolveUri(path);

      const String subPath = id->Subpath();
      if (subPath.empty())
      {
        auto data = Provider->GetData(id->Path());
        DetectCallback detectCallback(detectParams, Attributes, Provider, CoreParams, std::move(id));
        Module::Detect(*CoreParams, std::move(data), detectCallback);
      }
      else
      {
        OpenModule(path, detectParams);
      }
    }

    void OpenModule(const String& path, Playlist::Item::DetectParameters& detectParams) const override
    {
      auto id = IO::ResolveUri(path);

      auto data = Provider->GetData(id->Path());
      DetectCallback detectCallback(detectParams, Attributes, Provider, CoreParams, id);
      Module::Open(*CoreParams, std::move(data), id->Subpath(), detectCallback);
    }
  private:
    const CachedDataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr CoreParams;
    const DynamicAttributesProvider::Ptr Attributes;
  };
}

namespace Playlist
{
  namespace Item
  {
    DataProvider::Ptr DataProvider::Create(Parameters::Accessor::Ptr parameters)
    {
      return MakePtr<DataProviderImpl>(parameters);
    }
  }
}
