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
#include "playitems_provider.h"
#include "ui/utils.h"
//common includes
#include <error_tools.h>
#include <logging.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <io/fs_tools.h>
#include <io/provider.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#define FILE_TAG 0C9BBC6E

namespace
{
  typedef boost::shared_ptr<ZXTune::PluginInformation> PluginInfoPtr;
  
  //cached data provider
  class DataProvider
  {
  public:
    typedef boost::shared_ptr<DataProvider> Ptr;
    
    DataProvider() {}
    virtual ~DataProvider() {}

    virtual ZXTune::IO::DataContainer::Ptr GetData(const String& dataPath, const Parameters::Map& params, 
      const ZXTune::DetectParameters::LogFunc& logger)
    {
      //TODO: make cached
      ZXTune::IO::DataContainer::Ptr data;
      String subpath;
      ThrowIfError(ZXTune::IO::OpenData(dataPath, params, 
        logger ? boost::bind(&DataProvider::ConvertLog, _1, _2, boost::cref(logger)) : ZXTune::IO::ProgressCallback(),
        data, subpath));
      assert(subpath.empty());
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
  };
  
  struct SharedPlayitemContext
  {
    typedef boost::shared_ptr<SharedPlayitemContext> Ptr;
    
    SharedPlayitemContext()
    {
    }
    
    SharedPlayitemContext(const DataProvider::Ptr& provider, const Parameters::Map& params)
      : Provider(provider), CommonParams(params)
    {
    }
    DataProvider::Ptr Provider;
    Parameters::Map CommonParams;
  };

  Error AssignModule(const String& inSubpath, ZXTune::Module::Holder::Ptr inModule, 
                     const String& outSubpath, ZXTune::Module::Holder::Ptr& outModule)
  {
    if (inSubpath != outSubpath || outModule)
    {
      return Error(THIS_LINE, 1);//TODO
    }
    outModule = inModule;
    return Error();
  }
  
  //TODO: extract to core?
  Error OpenModule(const Parameters::Map& commonParams, ZXTune::IO::DataContainer::Ptr data, const String& subpath,
    ZXTune::Module::Holder::Ptr& result)
  {
    ZXTune::Module::Holder::Ptr tmp;
    ZXTune::DetectParameters params;
    params.Callback = boost::bind(&AssignModule, _1, _2, subpath, boost::ref(tmp));
    const Error& err = ZXTune::DetectModules(commonParams, params, data, subpath);
    if (tmp)
    {
      result = tmp;
      return Error();
    }
    return err ? err : Error(THIS_LINE, 1);//TODO
  }

  class PlayitemImpl : public Playitem
  {
  public:
    PlayitemImpl(
        //container-specific
        const SharedPlayitemContext::Ptr& context,
        //subset-specific
        const PluginInfoPtr& plugInfo, 
        //module-specific
        const String& dataPath, const String& subPath,
        const ZXTune::Module::Information& modInfo,
        const Parameters::Map& adjustedParams)
      : Context(context)
      , PluginInfo(plugInfo)
      , DataPath(dataPath), SubPath(subPath)
      , ModuleInfo(modInfo)
      , AdjustedParams(adjustedParams)
    {
    }
    
    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      ZXTune::IO::DataContainer::Ptr data = Context->Provider->GetData(DataPath, Context->CommonParams, 0);
      ZXTune::Module::Holder::Ptr result;
      ThrowIfError(OpenModule(Context->CommonParams, data, SubPath, result));
      return result;
    }
    
    virtual const ZXTune::Module::Information& GetModuleInfo() const
    {
      return ModuleInfo;
    }
    virtual const ZXTune::PluginInformation& GetPluginInfo() const
    {
      return *PluginInfo;
    }
    virtual const Parameters::Map& GetAdjustedParameters() const
    {
      return AdjustedParams;
    }
  private:
    const SharedPlayitemContext::Ptr Context;
    const PluginInfoPtr PluginInfo; 
    const String DataPath, SubPath;
    const ZXTune::Module::Information ModuleInfo;
    const Parameters::Map AdjustedParams;
  };
  
  class PlayitemsProviderImpl : public PlayitemsProvider
  {
    typedef std::map<String, PluginInfoPtr> PluginsMap;
  public:
    PlayitemsProviderImpl()
      : Provider(new DataProvider())
    {
      //fill plugins info map
      ZXTune::PluginInformationArray plugArray;
      ZXTune::EnumeratePlugins(plugArray);
      std::transform(plugArray.begin(), plugArray.end(), std::inserter(Plugins, Plugins.end()),
        boost::bind(&std::make_pair<PluginsMap::key_type, PluginsMap::mapped_type>,
          boost::bind(&ZXTune::PluginInformation::Id, _1),
          boost::bind(&boost::make_shared<ZXTune::PluginInformation, ZXTune::PluginInformation>, _1)));
    }
    
    virtual Error DetectModules(const String& path, 
                                const Parameters::Map& commonParams, const PlayitemDetectParameters& detectParams)
    {
      try
      {
        String dataPath, subPath;
        ThrowIfError(ZXTune::IO::SplitUri(path, dataPath, subPath));
        const ZXTune::IO::DataContainer::Ptr data = Provider->GetData(dataPath, commonParams, detectParams.Logger);
        
        const SharedPlayitemContext::Ptr context = 
          boost::make_shared<SharedPlayitemContext, DataProvider::Ptr, Parameters::Map>(Provider, commonParams);
          
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
  private:
    Error CreatePlayitem(const SharedPlayitemContext::Ptr& context,
      const String& dataPath, const String& subPath,
      const ZXTune::Module::Holder::Ptr& module,
      const PlayitemDetectParameters::CallbackFunc& callback)
    {
      try                                                                                                                                                      
      {                                                                                                                                                        
        //calculate full path
        String fullPath;                                                                                                                                              
        ThrowIfError(ZXTune::IO::CombineUri(dataPath, subPath, fullPath));
        //store custom attributes
        Parameters::Map attrs;                                                                                                                                   
        String tmp;                                                                                                                                              
        attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FILENAME,                                                                                  
          ZXTune::IO::ExtractLastPathComponent(dataPath, tmp)));                                                                                                     
        attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_PATH, subPath));                                                                              
        attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FULLPATH, fullPath));                                                                                
        ZXTune::Module::Information info;
        module->ModifyCustomAttributes(attrs, false);
        module->GetModuleInformation(info);
        ZXTune::PluginInformation plugInfo;
        module->GetPluginInformation(plugInfo);
        const PluginsMap::const_iterator plugIt = Plugins.find(plugInfo.Id);
        assert(plugIt != Plugins.end());
        const Playitem::Ptr item(new PlayitemImpl(
          context, plugIt->second, //common data
          dataPath, subPath, info, //item data
          Parameters::Map()        //TODO: adjusted parameters
        ));
        return callback(item) ? Error() : Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);                                                             
      }                                                                                                                                                        
      catch (const Error& e)                                                                                                                                   
      {                                                                                                                                                        
        return e;                                                                                                                                              
      }                                                                                                                                                        
    }
  private:
    const DataProvider::Ptr Provider;
    PluginsMap Plugins;
  };
}

PlayitemsProvider::Ptr PlayitemsProvider::Create()
{
  REGISTER_METATYPE(Playitem::Ptr);
  return PlayitemsProvider::Ptr(new PlayitemsProviderImpl());
}
