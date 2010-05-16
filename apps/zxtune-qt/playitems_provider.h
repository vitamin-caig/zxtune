/*
Abstract:
  Playlist data caching provider

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED
#define ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED

//library includes
#include <core/module_detect.h>

class Playitem
{
public:
  typedef boost::shared_ptr<Playitem> Ptr;
  
  virtual ~Playitem() {}
  
  virtual ZXTune::Module::Holder::Ptr GetModule() const = 0;
  virtual const ZXTune::Module::Information& GetModuleInfo() const = 0;
  virtual const ZXTune::PluginInformation& GetPluginInfo() const = 0;
  virtual const Parameters::Map& GetAdjustedParameters() const = 0;
};

struct PlayitemDetectParameters
{
  ZXTune::DetectParameters::FilterFunc Filter;
  typedef boost::function<bool(Playitem::Ptr)> CallbackFunc;
  CallbackFunc Callback;
  ZXTune::DetectParameters::LogFunc Logger;
};

class PlayitemsProvider
{
public:
  typedef std::auto_ptr<PlayitemsProvider> Ptr;
  
  virtual ~PlayitemsProvider() {}
  
  virtual Error DetectModules(const String& path, 
    const Parameters::Map& commonParams, const PlayitemDetectParameters& detectParams) = 0;
    
  static Ptr Create();
};

#endif //ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED
