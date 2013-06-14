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
    class PropertiesBuilder : public Parameters::Visitor, public Formats::Chiptune::MetaBuilder
    {
    public:
      PropertiesBuilder();

      virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val);
      virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val);
      virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val);

      virtual void SetProgram(const String& program);
      virtual void SetTitle(const String& title);
      virtual void SetAuthor(const String& author);

      void SetType(const String& type);
      void SetLocation(DataLocation::Ptr location);
      void SetSource(Binary::Data::Ptr data, std::size_t usedSize, const ModuleRegion& fixedRegion);
      void SetSource(Formats::Chiptune::Container::Ptr source);
      void SetComment(const String& comment);
      void SetFreqtable(const String& table);
      void SetSamplesFreq(uint_t freq);
      void SetVersion(uint_t major, uint_t minor);
      void SetVersion(const String& version);

      Parameters::Accessor::Ptr GetResult() const
      {
        return Container;
      }
    private:
      const Parameters::Container::Ptr Container;
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_MODULE_PROPERTIES_H_DEFINED__
