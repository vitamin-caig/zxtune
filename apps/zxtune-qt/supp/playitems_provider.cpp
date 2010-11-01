/*
Abstract:
  Playlist data caching provider implementation

Last changed:
  $Id: playitems_provider.cpp 511 2010-05-18 17:08:54Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playitems_provider.h"
#include "ui/format.h"
#include "ui/utils.h"
#include <apps/base/playitem.h>
//common includes
#include <error_tools.h>
#include <logging.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
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
  const std::string THIS_MODULE("QT::PlaylistProvider");

  //cached data provider
  class DataProvider
  {
    typedef std::map<String, ZXTune::IO::DataContainer::Ptr> CacheMap;
    typedef std::list<String> CacheList;
    typedef boost::unique_lock<boost::mutex> Locker;
    //TODO: parametrize
    static const uint64_t MAX_CACHE_SIZE = 1048576 * 10;//10Mb
    static const uint_t MAX_CACHE_ITEMS = 1000;
  public:
    typedef boost::shared_ptr<DataProvider> Ptr;

    DataProvider()
      : CacheSize(0)
    {
    }

    ZXTune::IO::DataContainer::Ptr GetData(const String& dataPath, const Parameters::Accessor& ioParams)
    {
      //check in cache
      {
        Locker lock(Mutex);
        const CacheMap::const_iterator cacheIt = Cache.find(dataPath);
        if (cacheIt != Cache.end())//has cache
        {
          Log::Debug(THIS_MODULE, "Getting '%1%' from cache", dataPath);
          CacheHistory.remove(dataPath);
          CacheHistory.push_front(dataPath);
          return cacheIt->second;
        }
      }
      //open data
      ZXTune::IO::DataContainer::Ptr data;
      {
        String subpath;
        ThrowIfError(ZXTune::IO::OpenData(dataPath, ioParams, ZXTune::IO::ProgressCallback(),
          data, subpath));
        assert(subpath.empty());
      }
      //store to cache
      Locker lock(Mutex);
      const uint64_t thisSize = data->Size();
      //check and reset cache
      while ((CacheSize + thisSize > MAX_CACHE_SIZE && !CacheHistory.empty()) ||
              Cache.size() + 1 > MAX_CACHE_ITEMS)
      {
        const String& removedItem(CacheHistory.back());
        const CacheMap::iterator cacheIt = Cache.find(removedItem);
        const uint64_t tmpSize = CacheSize - cacheIt->second->Size();
        Cache.erase(cacheIt);
        CacheHistory.remove(removedItem);
        CacheSize = tmpSize;
        Log::Debug(THIS_MODULE, "Removed '%1%' from cache (new size is %2%)", removedItem, CacheSize);
      }
      //fill cache
      Cache.insert(CacheMap::value_type(dataPath, data));
      CacheHistory.push_front(dataPath);
      CacheSize += thisSize;
      Log::Debug(THIS_MODULE, "Stored '%1%' to cache (new size is %2%)", dataPath, CacheSize);
      return data;
    }
  private:
    boost::mutex Mutex;
    CacheMap Cache;
    uint64_t CacheSize;
    CacheList CacheHistory;
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

  class PlayitemAttributesImpl : public PlayitemAttributes
  {
  public:
    explicit PlayitemAttributesImpl(const ZXTune::Module::Information& info)
      : Type(GetModuleType(*info.Properties()))
      , Title(GetModuleTitle(Text::MODULE_PLAYLIST_FORMAT, *info.Properties()))
      , Duration(info.FramesCount())
    {

    }

    virtual String GetType() const
    {
      return Type;
    }

    virtual String GetTitle() const
    {
      return Title;
    }

    virtual uint_t GetDuration() const
    {
      return Duration;
    }
  private:
    const String Type;
    const String Title;
    const uint_t Duration;
  };

  class PlayitemImpl : public Playitem
  {
  public:
    PlayitemImpl(
        //container-specific
        DataProvider::Ptr provider,
        //location-specific
        const String& dataPath, const String& subPath,
        //module-specific
        Parameters::Accessor::Ptr moduleParams,
        Parameters::Container::Ptr adjustedParams,
        PlayitemAttributes::Ptr attributes)
      : Provider(provider)
      , DataPath(dataPath), SubPath(subPath)
      , ModuleParams(moduleParams)
      , AdjustedParams(adjustedParams)
      , Attributes(attributes)
    {
    }

    virtual const PlayitemAttributes& GetAttributes() const
    {
      return *Attributes;
    }

    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      const Parameters::Accessor::Ptr finalParams = Parameters::CreateMergedAccessor(AdjustedParams, ModuleParams);
      const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(DataPath, *finalParams);
      ZXTune::Module::Holder::Ptr module;
      ThrowIfError(ZXTune::OpenModule(finalParams, data, SubPath, module));
      return module;
    }

    virtual Parameters::Accessor::Ptr GetAdjustedParameters() const
    {
      return AdjustedParams;
    }
  private:
    const DataProvider::Ptr Provider;
    const String DataPath, SubPath;
    const Parameters::Accessor::Ptr ModuleParams;
    const Parameters::Container::Ptr AdjustedParams;
    const PlayitemAttributes::Ptr Attributes;
  };

  class DetectParametersAdapter : public ZXTune::DetectParameters
  {
  public:
    DetectParametersAdapter(PlayitemDetectParameters& delegate,
                            DataProvider::Ptr provider, Parameters::Accessor::Ptr commonParams, const String& dataPath)
      : Delegate(delegate)
      , Provider(provider)
      , CommonParams(commonParams)
      , DataPath(dataPath)
    {
    }

    virtual bool FilterPlugin(const ZXTune::Plugin& /*plugin*/) const
    {
      //TODO: from parameters
      return false;
    }

    virtual Error ProcessModule(const String& subPath, ZXTune::Module::Holder::Ptr holder) const
    {
      const Parameters::Accessor::Ptr pathProperties = CreatePathProperties(DataPath, subPath);
      const ZXTune::Module::Information::Ptr originalInfo = holder->GetModuleInformation();
      const ZXTune::Module::Information::Ptr extendedInfo = CreateMixinPropertiesInformation(originalInfo, pathProperties);
      const PlayitemAttributes::Ptr attributes = boost::make_shared<PlayitemAttributesImpl>(*extendedInfo);
      //no adjusted parameters here- just empty container
      const Parameters::Container::Ptr adjustedParams = Parameters::Container::Create();
      const Parameters::Accessor::Ptr moduleParams = Parameters::CreateMergedAccessor(CommonParams, pathProperties);
      const Playitem::Ptr playitem = boost::make_shared<PlayitemImpl>(Provider,
          DataPath, subPath, moduleParams, adjustedParams, attributes);
      return Delegate.ProcessPlayitem(playitem) ? Error() : Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
    }

    virtual void ReportMessage(const Log::MessageData& message) const
    {
      Delegate.ShowProgress(message);
    }
  private:
    PlayitemDetectParameters& Delegate;
    const DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr CommonParams;
    const String DataPath;
  };

  class PlayitemsProviderImpl : public PlayitemsProvider
  {
  public:
    PlayitemsProviderImpl()
      : Provider(new DataProvider())
    {
    }

    virtual Error DetectModules(const String& path,
                                Parameters::Accessor::Ptr commonParams, PlayitemDetectParameters& detectParams)
    {
      try
      {
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath, *commonParams);

        const DetectParametersAdapter params(detectParams, Provider, commonParams, dataPath);
        ThrowIfError(ZXTune::DetectModules(commonParams, params, data, subPath));
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual void ResetCache()
    {
      Provider.reset(new DataProvider());
    }
  private:
    DataProvider::Ptr Provider;
  };
}

PlayitemsProvider::Ptr PlayitemsProvider::Create()
{
  return PlayitemsProvider::Ptr(new PlayitemsProviderImpl());
}
