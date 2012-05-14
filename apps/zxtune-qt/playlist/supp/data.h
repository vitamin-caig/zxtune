/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED

//common includes
#include <parameters.h>
#include <time_tools.h>
//library includes
#include <core/module_holder.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Playlist
{
  namespace Item
  {
    class Data
    {
    public:
      typedef boost::shared_ptr<const Data> Ptr;

      virtual ~Data() {}

      //common
      virtual ZXTune::Module::Holder::Ptr GetModule() const = 0;
      virtual Parameters::Container::Ptr GetAdjustedParameters() const = 0;
      //playlist-related
      virtual bool IsValid() const = 0;
      virtual String GetFullPath() const = 0;
      virtual String GetType() const = 0;
      virtual String GetDisplayName() const = 0;
      virtual Time::Milliseconds GetDuration() const = 0;
      virtual String GetDurationString() const = 0;
      virtual String GetTooltip() const = 0;
      virtual uint32_t GetChecksum() const = 0;
      virtual uint32_t GetCoreChecksum() const = 0;
      virtual std::size_t GetSize() const = 0;
    };

    class Callback
    {
    public:
      virtual ~Callback() {}

      virtual void OnItem(Item::Data::Ptr data) = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED
