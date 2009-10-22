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
  
  void RegisterPSGPlayer(PluginsEnumerator& enumerator);
  
  void RegisterPlayerPlugins(PluginsEnumerator& enumerator)
  {
    RegisterPSGPlayer(enumerator);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_LIST_H_DEFINED__
