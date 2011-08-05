/*
Abstract:
  Module properties container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__

//local includes
#include "core/plugins/enumerator.h"
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
      typedef boost::shared_ptr<const ModuleProperties> Ptr;
      typedef boost::shared_ptr<ModuleProperties> RWPtr;

      virtual void SetTitle(const String& title) = 0;
      virtual void SetAuthor(const String& author) = 0;
      virtual void SetComment(const String& comment) = 0;
      virtual void SetProgram(const String& program) = 0;
      virtual void SetWarnings(Log::MessagesCollector::Ptr warns) = 0;
      virtual void SetFreqtable(const String& table) = 0;
      virtual void SetVersion(uint_t major, uint_t minor) = 0;
      virtual void SetSource(std::size_t usedSize, const struct ModuleRegion& fixedRegion) = 0;

      virtual Plugin::Ptr GetPlugin() const = 0;
      virtual void GetData(Dump& dump) const = 0;

      static RWPtr Create(Plugin::Ptr plugin, DataLocation::Ptr location);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
