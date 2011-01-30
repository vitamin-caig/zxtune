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
  class PluginsEnumerator;

  void RegisterHobetaConvertor(PluginsEnumerator& enumerator);
  void RegisterHrust1xConvertor(PluginsEnumerator& enumerator);
  void RegisterHrust2xConvertor(PluginsEnumerator& enumerator);
  void RegisterFDIConvertor(PluginsEnumerator& enumerator);
  void RegisterDSQConvertor(PluginsEnumerator& enumerator);
  void RegisterMSPConvertor(PluginsEnumerator& enumerator);
  void RegisterTRUSHConvertor(PluginsEnumerator& enumerator);
  void RegisterLZSConvertor(PluginsEnumerator& enumerator);
  void RegisterPCDConvertor(PluginsEnumerator& enumerator);
  void RegisterHrumConvertor(PluginsEnumerator& enumerator);
  void RegisterCC3Convertor(PluginsEnumerator& enumerator);
  void RegisterESVConvertor(PluginsEnumerator& enumerator);

  void RegisterArchivePlugins(PluginsEnumerator& enumerator)
  {
    RegisterHobetaConvertor(enumerator);
    RegisterHrust1xConvertor(enumerator);
    RegisterHrust2xConvertor(enumerator);
    RegisterFDIConvertor(enumerator);
    RegisterDSQConvertor(enumerator);
    RegisterMSPConvertor(enumerator);
    RegisterTRUSHConvertor(enumerator);
    RegisterLZSConvertor(enumerator);
    RegisterPCDConvertor(enumerator);
    RegisterHrumConvertor(enumerator);
    RegisterCC3Convertor(enumerator);
    RegisterESVConvertor(enumerator);
  }
}

#endif //__CORE_PLUGINS_ARCHIVES_LIST_H_DEFINED__
