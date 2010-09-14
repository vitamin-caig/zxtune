/*
Abstract:
  Module properties container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__

//local includes
#include <core/plugins/enumerator.h>
//common includes
#include <messages_collector.h>
#include <parameters.h>
//library includes
#include <io/container.h>

namespace ZXTune
{
  namespace Module
  {
    class ModuleProperties : public Parameters::Accessor
    {
    public:
      typedef boost::shared_ptr<ModuleProperties> Ptr;

      virtual void SetTitle(const String& title) = 0;
      virtual void SetAuthor(const String& author) = 0;
      virtual void SetProgram(const String& program) = 0;
      virtual void SetWarnings(Log::MessagesCollector::Ptr warns) = 0;
      virtual void SetPath(const String& path) = 0;
      virtual void SetPlugins(PluginsChain::Ptr plugins) = 0;
      virtual void SetSource(IO::DataContainer::Ptr src, const struct ModuleRegion& fixedRegion) = 0;

      static Ptr Create(const String& pluginId);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
