/*
Abstract:
  Player plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsEnumerator;

  void RegisterTSSupport(PluginsEnumerator& enumerator);
  void RegisterPSGSupport(PluginsEnumerator& enumerator);
  void RegisterSTCSupport(PluginsEnumerator& enumerator);
  void RegisterPT2Support(PluginsEnumerator& enumerator);
  void RegisterPT3Support(PluginsEnumerator& enumerator);
  void RegisterASCSupport(PluginsEnumerator& enumerator);
  void RegisterSTPSupport(PluginsEnumerator& enumerator);
  void RegisterTXTSupport(PluginsEnumerator& enumerator);
  void RegisterPDTSupport(PluginsEnumerator& enumerator);
  void RegisterCHISupport(PluginsEnumerator& enumerator);

  void RegisterPlayerPlugins(PluginsEnumerator& enumerator)
  {
    //try TS first
    RegisterTSSupport(enumerator);
    RegisterPT3Support(enumerator);
    RegisterPT2Support(enumerator);
    RegisterSTCSupport(enumerator);
    RegisterASCSupport(enumerator);
    RegisterSTPSupport(enumerator);
    RegisterTXTSupport(enumerator);
    RegisterPSGSupport(enumerator);
    RegisterPDTSupport(enumerator);
    RegisterCHISupport(enumerator);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__
