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
    static const uint64_t MAX_CACHE_SIZE = 1048576 * 100;//100Mb
  public:
    typedef boost::shared_ptr<DataProvider> Ptr;
    
    DataProvider()
      : CacheSize(0)
    {
    }

    ZXTune::IO::DataContainer::Ptr GetData(const String& dataPath, const Parameters::Accessor& ioParams,
      const ZXTune::DetectParameters::LogFunc& logger)
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
        ThrowIfError(ZXTune::IO::OpenData(dataPath, ioParams,
          logger ? boost::bind(&DataProvider::ConvertLog, _1, _2, boost::cref(logger)) : ZXTune::IO::ProgressCallback(),
          data, subpath));
        assert(subpath.empty());
      }
      //store to cache
      Locker lock(Mutex);
      const uint64_t thisSize = data->Size();
      //check and reset cache
      while (CacheSize + thisSize > MAX_CACHE_SIZE &&
             !CacheHistory.empty())
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
    static Error ConvertLog(const String& msg, uint_t progress,
      const ZXTune::DetectParameters::LogFunc& logger)
    {
      assert(logger);
      Log::MessageData data;
      data.Text = msg;
      data.Progress = progress;
      logger(data);
      return Error();
    }
  private:
    boost::mutex Mutex;
    CacheMap Cache;
    uint64_t CacheSize;
    CacheList CacheHistory;
  };
  
  class SharedPlayitemContext
  {
  public:
    typedef boost::shared_ptr<SharedPlayitemContext> Ptr;

    SharedPlayitemContext(DataProvider::Ptr provider, Parameters::Accessor::Ptr params)
      : Provider(provider), CommonParams(params)
    {
    }

    ZXTune::Module::Holder::Ptr GetModule(const String& dataPath, const String& subPath, Parameters::Accessor::Ptr adjustedParams) const
    {
      const Parameters::Accessor::Ptr moduleParams = adjustedParams
        ? Parameters::CreateMergedAccessor(adjustedParams, CommonParams) : CommonParams;
      const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath, *moduleParams, 0/*logger*/);
      ZXTune::Module::Holder::Ptr module;
      ThrowIfError(ZXTune::OpenModule(moduleParams, data, subPath, module));
      const Parameters::Accessor::Ptr pathParams = CreatePathProperties(dataPath, subPath);
      const Parameters::Accessor::Ptr collectedModuleParams = Parameters::CreateMergedAccessor(moduleParams, pathParams);
      return CreateMixinPropertiesModule(module, collectedModuleParams, collectedModuleParams);
    }
  private:
    const DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr CommonParams;
  };

  class PlayitemImpl : public Playitem
  {
  public:
    PlayitemImpl(
        //container-specific
        SharedPlayitemContext::Ptr context,
        //location-specific
        const String& dataPath, const String& subPath,
        //module-specific
        Parameters::Accessor::Ptr adjustedParams)
      : Context(context)
      , DataPath(dataPath), SubPath(subPath)
      , AdjustedParams(adjustedParams)
    {
    }
    
    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      return Context->GetModule(DataPath, SubPath, AdjustedParams);
    }
    
    virtual ZXTune::Module::Information::Ptr GetModuleInfo() const
    {
      if (!ModuleInfo)
      {
        const ZXTune::Module::Holder::Ptr module = GetModule();
        ModuleInfo = module->GetModuleInformation();
      }
      return ModuleInfo;
    }
    
    virtual Parameters::Accessor::Ptr GetAdjustedParameters() const
    {
      return AdjustedParams;
    }
  private:
    const SharedPlayitemContext::Ptr Context;
    const String DataPath, SubPath;
    const Parameters::Accessor::Ptr AdjustedParams;
    mutable ZXTune::Module::Information::Ptr ModuleInfo;
  };
  
  class PlayitemsProviderImpl : public PlayitemsProvider
  {
  public:
    PlayitemsProviderImpl()
      : Provider(new DataProvider())
    {
    }
    
    virtual Error DetectModules(const String& path,
                                Parameters::Accessor::Ptr commonParams, const PlayitemDetectParameters& detectParams)
    {
      try
      {
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath, *commonParams, detectParams.Logger);
        
        const SharedPlayitemContext::Ptr context = boost::make_shared<SharedPlayitemContext>(Provider, commonParams);
          
        ZXTune::DetectParameters params;
        params.Filter = detectParams.Filter;
        params.Logger = detectParams.Logger;
        params.Callback = boost::bind(&PlayitemsProviderImpl::CreatePlayitem, this,
          context, dataPath, _1, _2, detectParams.Callback);
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
    Error CreatePlayitem(SharedPlayitemContext::Ptr context,
      const String& dataPath, const String& subPath,
      ZXTune::Module::Holder::Ptr /*module*/,
      const PlayitemDetectParameters::CallbackFunc& callback)
    {
      try
      {
        const Parameters::Accessor::Ptr adjustedParams;
        const Playitem::Ptr item(new PlayitemImpl(
          context, //common data
          dataPath, subPath, //item data
          adjustedParams
        ));
        return callback(item) ? Error() : Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  private:
    DataProvider::Ptr Provider;
  };
}

PlayitemsProvider::Ptr PlayitemsProvider::Create()
{
  REGISTER_METATYPE(Playitem::Ptr);
  return PlayitemsProvider::Ptr(new PlayitemsProviderImpl());
}
