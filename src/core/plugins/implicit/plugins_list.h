/*
Abstract:
  Implicit plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_IMPLICITS_LIST_H_DEFINED__
#define __CORE_PLUGINS_IMPLICITS_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsEnumerator;
  
  void RegisterHobetaConvertor(PluginsEnumerator& enumerator);
  
  void RegisterImplicitPlugins(PluginsEnumerator& enumerator)
  {
    RegisterHobetaConvertor(enumerator);
  }
}

#endif //__CORE_PLUGINS_IMPLICITS_LIST_H_DEFINED__
