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
#include "core/src/location.h"
//common includes
#include <parameters.h>
//library includes
#include <formats/chiptune.h>
#include <formats/chiptune/builder_meta.h>

namespace ZXTune
{
  struct ModuleRegion;

  namespace Module
  {
    class ModuleProperties : public Parameters::Accessor, public Formats::Chiptune::MetaBuilder
    {
    public:
      typedef boost::shared_ptr<const ModuleProperties> Ptr;
      typedef boost::shared_ptr<ModuleProperties> RWPtr;

      virtual void SetComment(const String& comment) = 0;
      virtual void SetFreqtable(const String& table) = 0;
      virtual void SetSamplesFreq(uint_t freq) = 0;
      virtual void SetVersion(uint_t major, uint_t minor) = 0;
      virtual void SetSource(std::size_t usedSize, const ModuleRegion& fixedRegion) = 0;
      virtual void SetSource(Formats::Chiptune::Container::Ptr source) = 0;

      virtual Parameters::Modifier::Ptr GetInternalContainer() const = 0;

      static RWPtr Create(const String& type, DataLocation::Ptr location);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
