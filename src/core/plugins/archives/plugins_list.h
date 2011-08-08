/*
Abstract:
  Archive plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ARCHIVES_LIST_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVES_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsRegistrator;

  void RegisterHobetaConvertor(PluginsRegistrator& registrator);
  void RegisterHrust1xConvertor(PluginsRegistrator& registrator);
  void RegisterHrust2xConvertor(PluginsRegistrator& registrator);
  void RegisterFDIConvertor(PluginsRegistrator& registrator);
  void RegisterDSQConvertor(PluginsRegistrator& registrator);
  void RegisterMSPConvertor(PluginsRegistrator& registrator);
  void RegisterTRUSHConvertor(PluginsRegistrator& registrator);
  void RegisterLZSConvertor(PluginsRegistrator& registrator);
  void RegisterPCDConvertor(PluginsRegistrator& registrator);
  void RegisterHrumConvertor(PluginsRegistrator& registrator);
  void RegisterCC3Convertor(PluginsRegistrator& registrator);
  void RegisterCC4Convertor(PluginsRegistrator& registrator);
  void RegisterESVConvertor(PluginsRegistrator& registrator);

  void RegisterArchivePlugins(PluginsRegistrator& registrator)
  {
    RegisterHobetaConvertor(registrator);
    RegisterHrust1xConvertor(registrator);
    RegisterHrust2xConvertor(registrator);
    RegisterFDIConvertor(registrator);
    RegisterDSQConvertor(registrator);
    RegisterMSPConvertor(registrator);
    RegisterTRUSHConvertor(registrator);
    RegisterLZSConvertor(registrator);
    RegisterPCDConvertor(registrator);
    RegisterHrumConvertor(registrator);
    RegisterCC3Convertor(registrator);
    RegisterCC4Convertor(registrator);
    RegisterESVConvertor(registrator);
  }
}

#endif //__CORE_PLUGINS_ARCHIVES_LIST_H_DEFINED__
