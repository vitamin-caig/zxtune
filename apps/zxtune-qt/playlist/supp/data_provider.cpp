/*
Abstract:
  Playlist data caching provider implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "data_provider.h"
#include "ui/format.h"
#include "ui/utils.h"
#include <apps/base/playitem.h>
//common includes
#include <error_tools.h>
#include <progress_callback.h>
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/provider.h>
#include <sound/sound_parameters.h>
#include <strings/format.h>
#include <strings/template.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 0C9BBC6E

namespace
{
  class DataProvider
  {
  public:
    typedef boost::shared_ptr<const DataProvider> Ptr;

    virtual ~DataProvider() {}

    virtual Binary::Container::Ptr GetData(const String& dataPath) const = 0;
  };

  class SimpleDataProvider : public DataProvider
  {
  public:
    explicit SimpleDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(ioParams)
    {
    }

    virtual Binary::Container::Ptr GetData(const String& dataPath) const
    {
      return ZXTune::IO::OpenData(dataPath, *Params, Log::ProgressCallback::Stub());
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  DataProvider::Ptr CreateSimpleDataProvider(Parameters::Accessor::Ptr ioParams)
  {
    return boost::make_shared<SimpleDataProvider>(ioParams);
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

      Item(const String& id, T val)
        : Id(id)
        , Value(val)
        , Weight(ObjectTraits<T>::Weight(val))
      {
      }
    };

    typedef std::list<Item> ItemsList;
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
      const typename ItemsList::iterator it = std::find_if(Items.begin(), Items.end(),
        boost::bind(&Item::Id, _1) == id);
      if (it != Items.end())
      {
        Items.erase(it);
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
  private:
    Item* FindItem(const String& id)
    {
      const typename ItemsList::iterator it = std::find_if(Items.begin(), Items.end(),
        boost::bind(&Item::Id, _1) == id);
      if (it != Items.end())
      {
        if (Items.size() > 1)
        {
          std::iter_swap(it, Items.begin());
        }
        return &Items.front();
      }
      return 0;
    }
  private:
    ItemsList Items;
    W TotalWeight;
  };

  //cached data provider
  class CachedDataProvider : public DataProvider
  {
    //TODO: parametrize
    static const uint64_t MAX_CACHE_SIZE = 1048576 * 10;//10Mb
    static const uint_t MAX_CACHE_ITEMS = 1000;
  public:
    typedef boost::shared_ptr<CachedDataProvider> Ptr;

    explicit CachedDataProvider(Parameters::Accessor::Ptr ioParams)
      : Delegate(CreateSimpleDataProvider(ioParams))
    {
    }

    virtual Binary::Container::Ptr GetData(const String& dataPath) const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      if (const Binary::Container::Ptr cached = Cache.Find(dataPath))
      {
        return cached;
      }
      const Binary::Container::Ptr data = Delegate->GetData(dataPath);
      Cache.Add(dataPath, data);
      Cache.Fit(MAX_CACHE_ITEMS, MAX_CACHE_SIZE);
      return data;
    }

    void FlushCachedData(const String& dataPath)
    {
      const boost::mutex::scoped_lock lock(Mutex);
      Cache.Del(dataPath);
    }
  private:
    const DataProvider::Ptr Delegate;
    mutable boost::mutex Mutex;
    mutable ObjectsCache<Binary::Container::Ptr> Cache;
  };

  class DataSource
  {
  public:
    typedef boost::shared_ptr<const DataSource> Ptr;

    DataSource(CachedDataProvider::Ptr provider, ZXTune::IO::Identifier::Ptr id)
      : Provider(provider)
      , DataId(id)
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

    ZXTune::IO::Identifier::Ptr GetDataIdentifier() const
    {
      return DataId;
    }
  private:
    const CachedDataProvider::Ptr Provider;
    const ZXTune::IO::Identifier::Ptr DataId;
  };

  class ModuleSource
  {
  public:
    ModuleSource(Parameters::Accessor::Ptr coreParams, DataSource::Ptr source, ZXTune::IO::Identifier::Ptr moduleId)
      : CoreParams(coreParams)
      , Source(source)
      , ModuleId(moduleId)
    {
    }

    ZXTune::Module::Holder::Ptr GetModule(Parameters::Accessor::Ptr adjustedParams) const
    {
      try
      {
        const Binary::Container::Ptr data = Source->GetData();
        const ZXTune::Module::Holder::Ptr module = ZXTune::OpenModule(CoreParams, data, ModuleId->Subpath());
        const Parameters::Accessor::Ptr pathParams = CreatePathProperties(ModuleId);
        const Parameters::Accessor::Ptr moduleParams = Parameters::CreateMergedAccessor(pathParams, adjustedParams);
        return ZXTune::Module::CreateMixedPropertiesHolder(module, moduleParams);
      }
      catch (const Error&)
      {
        return ZXTune::Module::Holder::Ptr();
      }
    }

    String GetFullPath() const
    {
      return ModuleId->Full();
    }
  private:
    const Parameters::Accessor::Ptr CoreParams;
    const DataSource::Ptr Source;
    const ZXTune::IO::Identifier::Ptr ModuleId;
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
    typedef boost::shared_ptr<const DynamicAttributesProvider> Ptr;

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
        if (!properties.FindValue(ZXTune::Module::ATTR_FULLPATH, result))
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
                 , private Parameters::PropertyChangedCallback
  {
  public:
    DataImpl(DynamicAttributesProvider::Ptr attributes,
        const ModuleSource& source,
        Parameters::Container::Ptr adjustedParams,
        uint_t frames, const Parameters::Accessor& moduleProps)
      : Attributes(attributes)
      , Source(source)
      , AdjustedParams(adjustedParams)
      , Type(GetStringProperty(moduleProps, ZXTune::Module::ATTR_TYPE))
      , Checksum(static_cast<uint32_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_CRC)))
      , CoreChecksum(static_cast<uint32_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_FIXEDCRC)))
      , Size(static_cast<std::size_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_SIZE)))
      , Valid(true)
    {
      Duration.SetCount(frames);
      LoadProperties(moduleProps);
    }

    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      const ZXTune::Module::Holder::Ptr res = Source.GetModule(AdjustedParams);
      Valid = res;
      return res;
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      const Parameters::PropertyChangedCallback& cb = *this;
      return Parameters::CreatePropertyTrackedContainer(AdjustedParams, cb);
    }

    //playlist-related properties
    virtual bool IsValid() const
    {
      return Valid;
    }

    virtual String GetFullPath() const
    {
      return Source.GetFullPath();
    }

    virtual String GetType() const
    {
      return Type;
    }

    virtual String GetDisplayName() const
    {
      return DisplayName;
    }

    virtual Time::MillisecondsDuration GetDuration() const
    {
      return Duration;
    }

    virtual String GetAuthor() const
    {
      return Author;
    }

    virtual String GetTitle() const
    {
      return Title;
    }

    virtual uint32_t GetChecksum() const
    {
      return Checksum;
    }

    virtual uint32_t GetCoreChecksum() const
    {
      return CoreChecksum;
    }

    virtual std::size_t GetSize() const
    {
      return Size;
    }
  private:
    Parameters::Accessor::Ptr GetModuleProperties() const
    {
      if (const ZXTune::Module::Holder::Ptr holder = GetModule())
      {
        return holder->GetModuleProperties();
      }
      return Parameters::Accessor::Ptr();
    }
  private:
    virtual void OnPropertyChanged(const Parameters::NameType& /*name*/) const
    {
      if (const Parameters::Accessor::Ptr properties = GetModuleProperties())
      {
        LoadProperties(*properties);
      }
      else
      {
        DisplayName.clear();
        Author.clear();
        Title.clear();
        Duration.SetCount(0);
      }
    }

    void LoadProperties(const Parameters::Accessor& props) const
    {
      DisplayName = Attributes->GetDisplayName(props);
      Author = GetStringProperty(props, ZXTune::Module::ATTR_AUTHOR);
      Title = GetStringProperty(props, ZXTune::Module::ATTR_TITLE);
      const Time::Microseconds period(GetIntProperty(props, Parameters::ZXTune::Sound::FRAMEDURATION, Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT));
      Duration.SetPeriod(period);
    }
  private:
    const DynamicAttributesProvider::Ptr Attributes;
    const ModuleSource Source;
    const Parameters::Container::Ptr AdjustedParams;
    const String Type;
    const uint32_t Checksum;
    const uint32_t CoreChecksum;
    const std::size_t Size;
    mutable String DisplayName;
    mutable String Author;
    mutable String Title;
    mutable Time::MillisecondsDuration Duration;
    mutable bool Valid;
  };

  class ProgressCallbackAdapter : public Log::ProgressCallback
  {
  public:
    explicit ProgressCallbackAdapter(Playlist::Item::DetectParameters& delegate)
      : Delegate(delegate)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      Delegate.ShowProgress(current);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      Delegate.ShowProgress(current);
      Delegate.ShowMessage(message);
    }
  private:
    Playlist::Item::DetectParameters& Delegate;
  };
 

  class DetectParametersAdapter : public ZXTune::DetectParameters
  {
  public:
    DetectParametersAdapter(Playlist::Item::DetectParameters& delegate,
                            DynamicAttributesProvider::Ptr attributes,
                            CachedDataProvider::Ptr provider, Parameters::Accessor::Ptr coreParams, ZXTune::IO::Identifier::Ptr dataId)
      : Delegate(delegate)
      , ProgressCallback(Delegate)
      , Attributes(attributes)
      , CoreParams(coreParams)
      , DataId(dataId)
      , Source(boost::make_shared<DataSource>(provider, dataId))
    {
    }

    virtual void ProcessModule(const String& subPath, ZXTune::Module::Holder::Ptr holder) const
    {
      const Parameters::Container::Ptr adjustedParams = Delegate.CreateInitialAdjustedParameters();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr moduleProps = holder->GetModuleProperties();
      const ZXTune::IO::Identifier::Ptr moduleId = DataId->WithSubpath(subPath);
      const Parameters::Accessor::Ptr pathProps = CreatePathProperties(moduleId);
      const Parameters::Accessor::Ptr lookupModuleProps = Parameters::CreateMergedAccessor(pathProps, adjustedParams, moduleProps);
      const ModuleSource itemSource(CoreParams, Source, moduleId);
      const Playlist::Item::Data::Ptr playitem = boost::make_shared<DataImpl>(Attributes, itemSource, adjustedParams,
        info->FramesCount(), *lookupModuleProps);
      Delegate.ProcessItem(playitem);
    }

    virtual Log::ProgressCallback* GetProgressCallback() const
    {
      return &ProgressCallback;
    }
  private:
    Playlist::Item::DetectParameters& Delegate;
    mutable ProgressCallbackAdapter ProgressCallback;
    const DynamicAttributesProvider::Ptr Attributes;
    const Parameters::Accessor::Ptr CoreParams;
    const ZXTune::IO::Identifier::Ptr DataId;
    const DataSource::Ptr Source;
  };

  class DataProviderImpl : public Playlist::Item::DataProvider
  {
  public:
    explicit DataProviderImpl(Parameters::Accessor::Ptr parameters)
      : Provider(new CachedDataProvider(parameters))
      , CoreParams(parameters)
      , Attributes(boost::make_shared<DynamicAttributesProvider>())
    {
    }

    virtual Error DetectModules(const String& path, Playlist::Item::DetectParameters& detectParams) const
    {
      try
      {
        const ZXTune::IO::Identifier::Ptr id = ZXTune::IO::ResolveUri(path);

        const String dataPath = id->Path();
        const Binary::Container::Ptr data = Provider->GetData(dataPath);
        const DetectParametersAdapter params(detectParams, Attributes, Provider, CoreParams, id);

        const String subPath = id->Subpath();
        ZXTune::DetectModules(CoreParams, params, data, subPath);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error OpenModule(const String& path, Playlist::Item::DetectParameters& detectParams) const
    {
      try
      {
        const ZXTune::IO::Identifier::Ptr id = ZXTune::IO::ResolveUri(path);

        const String dataPath = id->Path();
        const Binary::Container::Ptr data = Provider->GetData(dataPath);
        const DetectParametersAdapter params(detectParams, Attributes, Provider, CoreParams, id);

        const String subPath = id->Subpath();
        const ZXTune::Module::Holder::Ptr result = ZXTune::OpenModule(CoreParams, data, subPath);
        params.ProcessModule(subPath, result);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
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
      return boost::make_shared<DataProviderImpl>(parameters);
    }
  }
}
