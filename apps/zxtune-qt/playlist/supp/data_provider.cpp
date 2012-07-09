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
#include <format.h>
#include <logging.h>
#include <template_parameters.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/fs_tools.h>
#include <io/provider.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string/replace.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 0C9BBC6E

namespace
{
  const std::string THIS_MODULE("Playlist::DataProvider");

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
      Binary::Container::Ptr data;
      ThrowIfError(ZXTune::IO::OpenData(dataPath, *Params, ZXTune::IO::ProgressCallback(), data));
      return data;
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

    DataSource(CachedDataProvider::Ptr provider, const String& dataPath)
      : Provider(provider)
      , DataPath(dataPath)
    {
    }

    ~DataSource()
    {
      Provider->FlushCachedData(DataPath);
    }

    Binary::Container::Ptr GetData() const
    {
      return Provider->GetData(DataPath);
    }

    String GetDataPath() const
    {
      return DataPath;
    }
  private:
    const CachedDataProvider::Ptr Provider;
    const String DataPath;
  };

  class ModuleSource
  {
  public:
    ModuleSource(Parameters::Accessor::Ptr coreParams, DataSource::Ptr source, const String& subPath)
      : CoreParams(coreParams)
      , Source(source)
      , SubPath(subPath)
    {
    }

    ZXTune::Module::Holder::Ptr GetModule(Parameters::Accessor::Ptr adjustedParams) const
    {
      const Binary::Container::Ptr data = Source->GetData();
      ZXTune::Module::Holder::Ptr module;
      ZXTune::OpenModule(CoreParams, data, SubPath, module);
      if (module)
      {
        const Parameters::Accessor::Ptr pathParams = CreatePathProperties(Source->GetDataPath(), SubPath);
        const Parameters::Accessor::Ptr moduleParams = Parameters::CreateMergedAccessor(pathParams, adjustedParams);
        return ZXTune::Module::CreateMixedPropertiesHolder(module, moduleParams);
      }
      return module;
    }

    String GetFullPath() const
    {
      String result;
      ThrowIfError(ZXTune::IO::CombineUri(Source->GetDataPath(), SubPath, result));
      return result;
    }
  private:
    const Parameters::Accessor::Ptr CoreParams;
    const DataSource::Ptr Source;
    const String SubPath;
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

  Parameters::IntType GetIntProperty(const Parameters::Accessor& props, const Parameters::NameType& propName)
  {
    Parameters::IntType val = 0;
    if (props.FindValue(propName, val))
    {
      return val;
    }
    assert(!"Failed to get property");
    return 0;
  }

  class HTMLEscapedFieldsSourceAdapter : public Parameters::FieldsSourceAdapter<SkipFieldsSource>
  {
    typedef Parameters::FieldsSourceAdapter<SkipFieldsSource> Parent;
  public:
    explicit HTMLEscapedFieldsSourceAdapter(const Parameters::Accessor& props)
      : Parent(props)
    {
    }

    virtual String GetFieldValue(const String& fieldName) const
    {
      static const Char AMPERSAND[] = {'&', 0};
      static const Char AMPERSAND_ESCAPED[] = {'&', 'a', 'm', 'p', ';', 0};
      static const Char LBRACKET[] = {'<', 0};
      static const Char LBRACKET_ESCAPED[] = {'&', 'l', 't', ';', 0};
      String result = Parent::GetFieldValue(fieldName);
      boost::algorithm::replace_all(result, AMPERSAND, AMPERSAND_ESCAPED);
      boost::algorithm::replace_all(result, LBRACKET, LBRACKET_ESCAPED);
      return result;
    }
  };

  class DynamicAttributesProvider
  {
  public:
    typedef boost::shared_ptr<const DynamicAttributesProvider> Ptr;

    DynamicAttributesProvider()
      : DisplayNameTemplate(StringTemplate::Create(Text::MODULE_PLAYLIST_FORMAT))
      , DummyDisplayName(DisplayNameTemplate->Instantiate(SkipFieldsSource()))
      , TooltipTemplate(StringTemplate::Create(Text::TOOLTIP_TEMPLATE))
    {
    }

    String GetDisplayName(const Parameters::Accessor& properties) const
    {
      const Parameters::FieldsSourceAdapter<SkipFieldsSource> adapter(properties);
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

    String GetTooltip(const Parameters::Accessor& properties) const
    {
      const HTMLEscapedFieldsSourceAdapter adapter(properties);
      return TooltipTemplate->Instantiate(adapter);
    }
  private:
    const StringTemplate::Ptr DisplayNameTemplate;
    const String DummyDisplayName;
    const StringTemplate::Ptr TooltipTemplate;
  };

  class DataImpl : public Playlist::Item::Data
                 , private Parameters::PropertyChangedCallback
  {
  public:
    DataImpl(DynamicAttributesProvider::Ptr attributes,
        const ModuleSource& source,
        Parameters::Container::Ptr adjustedParams,
        unsigned duration, const Parameters::Accessor& moduleProps)
      : Attributes(attributes)
      , Source(source)
      , AdjustedParams(adjustedParams)
      , Type(GetStringProperty(moduleProps, ZXTune::Module::ATTR_TYPE))
      , DisplayName(Attributes->GetDisplayName(moduleProps))
      , Author(GetStringProperty(moduleProps, ZXTune::Module::ATTR_AUTHOR))
      , Title(GetStringProperty(moduleProps, ZXTune::Module::ATTR_TITLE))
      , DurationInFrames(duration)
      , Checksum(static_cast<uint32_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_CRC)))
      , CoreChecksum(static_cast<uint32_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_FIXEDCRC)))
      , Size(static_cast<std::size_t>(GetIntProperty(moduleProps, ZXTune::Module::ATTR_SIZE)))
      , Valid(true)
    {
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
      if (DisplayName.empty())
      {
        AcquireDisplayName();
      }
      return DisplayName;
    }

    virtual Time::Milliseconds GetDuration() const
    {
      //TODO:
      const unsigned FRAME_DURATION_MS = 20;
      return Time::Milliseconds(DurationInFrames * FRAME_DURATION_MS);
    }

    virtual String GetDurationString() const
    {
      return Strings::FormatTime(DurationInFrames, 20000);//TODO
    }

    virtual String GetTooltip() const
    {
      if (const Parameters::Accessor::Ptr properties = GetModuleProperties())
      {
        return Attributes->GetTooltip(*properties);
      }
      return String();
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
    void AcquireDisplayName() const
    {
      if (const Parameters::Accessor::Ptr properties = GetModuleProperties())
      {
        DisplayName = Attributes->GetDisplayName(*properties);
      }
    }

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
      DisplayName.clear();
      if (const Parameters::Accessor::Ptr properties = GetModuleProperties())
      {
        Author = GetStringProperty(*properties, ZXTune::Module::ATTR_AUTHOR);
        Title = GetStringProperty(*properties, ZXTune::Module::ATTR_TITLE);
      }
      else
      {
        Author.clear();
        Title.clear();
      }
    }
  private:
    const DynamicAttributesProvider::Ptr Attributes;
    const ModuleSource Source;
    const Parameters::Container::Ptr AdjustedParams;
    const String Type;
    mutable String DisplayName;
    mutable String Author;
    mutable String Title;
    const unsigned DurationInFrames;
    const uint32_t Checksum;
    const uint32_t CoreChecksum;
    const std::size_t Size;
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
                            CachedDataProvider::Ptr provider, Parameters::Accessor::Ptr coreParams, const String& dataPath)
      : Delegate(delegate)
      , ProgressCallback(Delegate)
      , Attributes(attributes)
      , CoreParams(coreParams)
      , DataPath(dataPath)
      , Source(boost::make_shared<DataSource>(provider, dataPath))
    {
    }

    virtual void ProcessModule(const String& subPath, ZXTune::Module::Holder::Ptr holder) const
    {
      const Parameters::Container::Ptr adjustedParams = Delegate.CreateInitialAdjustedParameters();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr moduleProps = holder->GetModuleProperties();
      const Parameters::Accessor::Ptr pathProps = CreatePathProperties(DataPath, subPath);
      const Parameters::Accessor::Ptr lookupModuleProps = Parameters::CreateMergedAccessor(pathProps, adjustedParams, moduleProps);
      const ModuleSource itemSource(CoreParams, Source, subPath);
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
    const String DataPath;
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
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const Binary::Container::Ptr data = Provider->GetData(dataPath);

        const DetectParametersAdapter params(detectParams, Attributes, Provider, CoreParams, dataPath);
        ThrowIfError(ZXTune::DetectModules(CoreParams, params, data, subPath));
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
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const Binary::Container::Ptr data = Provider->GetData(dataPath);

        const DetectParametersAdapter params(detectParams, Attributes, Provider, CoreParams, dataPath);
        ZXTune::Module::Holder::Ptr result;
        ThrowIfError(ZXTune::OpenModule(CoreParams, data, subPath, result));
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
