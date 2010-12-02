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
#include <formatter.h>
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
//text includes
#include "text/text.h"

#define FILE_TAG 0C9BBC6E

namespace
{
  const std::string THIS_MODULE("Playlist::DataProvider");

  //cached data provider
  class CachedDataProvider
  {
    typedef std::pair<String, ZXTune::IO::DataContainer::Ptr> CacheEntry;
    typedef std::list<CacheEntry> CacheList;
    typedef boost::unique_lock<boost::mutex> Locker;
    //TODO: parametrize
    static const uint64_t MAX_CACHE_SIZE = 1048576 * 10;//10Mb
    static const uint_t MAX_CACHE_ITEMS = 1000;
  public:
    typedef boost::shared_ptr<CachedDataProvider> Ptr;

    explicit CachedDataProvider(Parameters::Accessor::Ptr ioParams)
      : Params(ioParams)
      , CacheSize(0)
    {
    }

    ZXTune::IO::DataContainer::Ptr GetData(const String& dataPath)
    {
      {
        Locker lock(Mutex);
        if (const ZXTune::IO::DataContainer::Ptr cached = FindCachedData(dataPath))
        {
          return cached;
        }
      }
      const ZXTune::IO::DataContainer::Ptr data = OpenNewData(dataPath);
      //store to cache
      Locker lock(Mutex);
      StoreToCache(dataPath, data);
      FitCache();
      return data;
    }

    void FlushCachedData(const String& dataPath)
    {
      Locker lock(Mutex);
      const CacheList::iterator cacheIt = std::find_if(Cache.begin(), Cache.end(),
        boost::bind(&CacheList::value_type::first, _1) == dataPath);
      if (cacheIt != Cache.end())
      {
        const CacheEntry entry = *cacheIt;
        Cache.erase(cacheIt);
        RemoveEntry(entry);
      }
    }
  private:
    ZXTune::IO::DataContainer::Ptr FindCachedData(const String& dataPath) const
    {
      const CacheList::iterator cacheIt = std::find_if(Cache.begin(), Cache.end(),
        boost::bind(&CacheList::value_type::first, _1) == dataPath);
      if (cacheIt != Cache.end())//has cache
      {
        const ZXTune::IO::DataContainer::Ptr result = cacheIt->second;
        Log::Debug(THIS_MODULE, "Getting '%1%' (%2% bytes) from cache", dataPath, result->Size());
        if (Cache.size() > 1)
        {
          std::swap(Cache.front(), *cacheIt);
        }
        return result;
      }
      return ZXTune::IO::DataContainer::Ptr();
    }

    ZXTune::IO::DataContainer::Ptr OpenNewData(const String& dataPath) const
    {
      ZXTune::IO::DataContainer::Ptr data;
      String subpath;
      ThrowIfError(ZXTune::IO::OpenData(dataPath, *Params, ZXTune::IO::ProgressCallback(),
        data, subpath));
      assert(subpath.empty());
      return data;
    }

    void StoreToCache(const String& dataPath, const ZXTune::IO::DataContainer::Ptr& data) const
    {
      Cache.push_front(CacheList::value_type(dataPath, data));
      const uint64_t size = data->Size();
      CacheSize += size;
      Log::Debug(THIS_MODULE, "Stored '%1%' (size %2%) to cache (cache size %3%/%4% bytes)", dataPath, size, Cache.size(), CacheSize);
    }

    void FitCache()
    {
      while (CacheSize > MAX_CACHE_SIZE ||
             Cache.size() > MAX_CACHE_ITEMS)
      {
        const CacheEntry entry = Cache.back();
        Cache.pop_back();
        RemoveEntry(entry);
      }
    }

    void RemoveEntry(const CacheEntry& entry)
    {
      const uint64_t size = entry.second->Size();
      CacheSize -= size;
      Log::Debug(THIS_MODULE, "Removed '%1%' (%2% bytes) from cache (cache size is %3%/%4% bytes)", entry.first, size, Cache.size(), CacheSize);
    }
  private:
    const Parameters::Accessor::Ptr Params;
    mutable boost::mutex Mutex;
    mutable CacheList Cache;
    mutable uint64_t CacheSize;
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

    ZXTune::IO::DataContainer::Ptr GetData() const
    {
      return Provider->GetData(DataPath);
    }
  private:
    const CachedDataProvider::Ptr Provider;
    const String DataPath;
  };

  class ModuleSource
  {
  public:
    ModuleSource(DataSource::Ptr source, Parameters::Accessor::Ptr moduleParams)
      : Source(source)
      , ModuleParams(moduleParams)
    {
    }

    ZXTune::Module::Holder::Ptr GetModule() const
    {
      const ZXTune::IO::DataContainer::Ptr data = Source->GetData();
      const String subPath = GetSubpath();
      ZXTune::Module::Holder::Ptr module;
      ThrowIfError(ZXTune::OpenModule(ModuleParams, data, subPath, module));
      return module;
    }
  private:
    String GetSubpath() const
    {
      Parameters::StringType subpath;
      ModuleParams->FindStringValue(ZXTune::Module::ATTR_SUBPATH, subpath);
      return subpath;
    }
  private:
    const DataSource::Ptr Source;
    const Parameters::Accessor::Ptr ModuleParams;
  };

  String GetModuleType(const Parameters::Accessor& props)
  {
    Parameters::StringType typeStr;
    if (props.FindStringValue(ZXTune::Module::ATTR_TYPE, typeStr))
    {
      return typeStr;
    }
    assert(!"Invalid module type");
    return String();
  }

  class DynamicAttributesProvider
  {
  public:
    typedef boost::shared_ptr<const DynamicAttributesProvider> Ptr;

    DynamicAttributesProvider()
      : TitleTemplate(StringTemplate::Create(Text::MODULE_PLAYLIST_FORMAT))
      , DummyTitle(TitleTemplate->Instantiate(SkipFieldsSource()))
      , TooltipTemplate(StringTemplate::Create(Text::TOOLTIP_TEMPLATE))
    {
    }

    String GetTitle(const Parameters::Accessor& properties) const
    {
      const Parameters::FieldsSourceAdapter<SkipFieldsSource> adapter(properties);
      String result = TitleTemplate->Instantiate(adapter);
      if (result == DummyTitle)
      {
        properties.FindStringValue(ZXTune::Module::ATTR_FULLPATH, result);
      }
      return result;
    }

    String GetTooltip(const Parameters::Accessor& properties) const
    {
      const Parameters::FieldsSourceAdapter<SkipFieldsSource> adapter(properties);
      return TooltipTemplate->Instantiate(adapter);
    }
  private:
    const StringTemplate::Ptr TitleTemplate;
    const String DummyTitle;
    const StringTemplate::Ptr TooltipTemplate;
  };

  class DataImpl : public Playlist::Item::Data
  {
  public:
    DataImpl(DynamicAttributesProvider::Ptr attributes,
        const ModuleSource& source,
        Parameters::Container::Ptr adjustedParams,
        unsigned duration, const Parameters::Accessor& moduleProps)
      : Attributes(attributes)
      , Source(source)
      , AdjustedParams(adjustedParams)
      , Type(GetModuleType(moduleProps))
      , Title(Attributes->GetTitle(moduleProps))
      , DurationInFrames(duration)
    {
    }

    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      return Source.GetModule();
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      Title.clear();
      return AdjustedParams;
    }

    //playlist-related properties
    virtual bool IsValid() const
    {
      return true;
    }

    virtual String GetType() const
    {
      return Type;
    }

    virtual String GetTitle() const
    {
      if (Title.empty())
      {
        AcquireTitle();
      }
      return Title;
    }

    virtual unsigned GetDurationValue() const
    {
      return DurationInFrames;
    }

    virtual String GetDurationString() const
    {
      return FormatTime(DurationInFrames, 20000);//TODO
    }

    virtual String GetTooltip() const
    {
      const Parameters::Accessor::Ptr properties = GetModuleProperties();
      return Attributes->GetTooltip(*properties);
    }
  private:
    void AcquireTitle() const
    {
      const Parameters::Accessor::Ptr properties = GetModuleProperties();
      Title = Attributes->GetTitle(*properties);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const
    {
      const ZXTune::Module::Holder::Ptr holder = Source.GetModule();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      return info->Properties();
    }
  private:
    const DynamicAttributesProvider::Ptr Attributes;
    const ModuleSource Source;
    const Parameters::Container::Ptr AdjustedParams;
    const String Type;
    mutable String Title;
    const unsigned DurationInFrames;
  };

  class DetectParametersAdapter : public ZXTune::DetectParameters
  {
  public:
    DetectParametersAdapter(Playlist::Item::DetectParameters& delegate,
                            DynamicAttributesProvider::Ptr attributes,
                            CachedDataProvider::Ptr provider, Parameters::Accessor::Ptr coreParams, const String& dataPath)
      : Delegate(delegate)
      , Attributes(attributes)
      , CoreParams(coreParams)
      , DataPath(dataPath)
      , Source(boost::make_shared<DataSource>(provider, dataPath))
    {
    }

    virtual bool FilterPlugin(const ZXTune::Plugin& /*plugin*/) const
    {
      //TODO: from parameters
      return false;
    }

    virtual Error ProcessModule(const String& subPath, ZXTune::Module::Holder::Ptr holder) const
    {
      const Parameters::Accessor::Ptr pathProps = CreatePathProperties(DataPath, subPath);
      const Parameters::Container::Ptr adjustedParams = Parameters::Container::Create();
      const Parameters::Accessor::Ptr perItemParameters = Parameters::CreateMergedAccessor(adjustedParams, CoreParams);
  
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const ModuleSource itemSource(Source, Parameters::CreateMergedAccessor(pathProps, perItemParameters));
      const Parameters::Accessor::Ptr moduleProps = Parameters::CreateMergedAccessor(pathProps, info->Properties());
      const Playlist::Item::Data::Ptr playitem = boost::make_shared<DataImpl>(Attributes, itemSource, adjustedParams,
        info->FramesCount(), *moduleProps);
      return Delegate.ProcessItem(playitem) ? Error() : Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
    }

    virtual void ReportMessage(const Log::MessageData& message) const
    {
      if (0 != message.Level)
      {
        return;
      }
      if (message.Text)
      {
        Delegate.ShowMessage(*message.Text);
      }
      if (message.Progress)
      {
        Delegate.ShowProgress(*message.Progress);
      }
    }
  private:
    Playlist::Item::DetectParameters& Delegate;
    const DynamicAttributesProvider::Ptr Attributes;
    const Parameters::Accessor::Ptr CoreParams;
    const String DataPath;
    const DataSource::Ptr Source;
  };

  class DataProviderImpl : public Playlist::Item::DataProvider
  {
  public:
    DataProviderImpl(Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
      : Provider(new CachedDataProvider(ioParams))
      , CoreParams(coreParams)
      , Attributes(boost::make_shared<DynamicAttributesProvider>()) 
    {
    }

    virtual Error DetectModules(const String& path, Playlist::Item::DetectParameters& detectParams) const
    {
      try
      {
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath);

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
        const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath);

        const DetectParametersAdapter params(detectParams, Attributes, Provider, CoreParams, dataPath);
        ZXTune::Module::Holder::Ptr result;
        ThrowIfError(ZXTune::OpenModule(CoreParams, data, subPath, result));
        ThrowIfError(params.ProcessModule(subPath, result));
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
    DataProvider::Ptr DataProvider::Create(Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams)
    {
      return boost::make_shared<DataProviderImpl>(ioParams, coreParams);
    }
  }
}
