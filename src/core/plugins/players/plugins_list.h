/*
Abstract:
  Player plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsRegistrator;

  void RegisterTSSupport(PluginsRegistrator& enumerator);
  void RegisterAYSupport(PluginsRegistrator& enumerator);
  void RegisterPSGSupport(PluginsRegistrator& enumerator);
  void RegisterSTCSupport(PluginsRegistrator& enumerator);
  void RegisterST1Support(PluginsRegistrator& enumerator);
  void RegisterPT2Support(PluginsRegistrator& enumerator);
  void RegisterPT3Support(PluginsRegistrator& enumerator);
  void RegisterASCSupport(PluginsRegistrator& enumerator);
  void RegisterSTPSupport(PluginsRegistrator& enumerator);
  void RegisterTXTSupport(PluginsRegistrator& enumerator);
  void RegisterPDTSupport(PluginsRegistrator& enumerator);
  void RegisterCHISupport(PluginsRegistrator& enumerator);

  void RegisterPlayerPlugins(PluginsRegistrator& enumerator)
  {
    //try TS & AY first
    RegisterTSSupport(enumerator);
    RegisterAYSupport(enumerator);
    RegisterPT3Support(enumerator);
    RegisterPT2Support(enumerator);
    RegisterSTCSupport(enumerator);
    RegisterST1Support(enumerator);
    RegisterASCSupport(enumerator);
    RegisterSTPSupport(enumerator);
    RegisterTXTSupport(enumerator);
    RegisterPSGSupport(enumerator);
    RegisterPDTSupport(enumerator);
    RegisterCHISupport(enumerator);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__
