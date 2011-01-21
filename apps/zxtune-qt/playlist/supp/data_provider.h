/*
Abstract:
  Playlist data caching provider

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_DATA_PROVIDER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_DATA_PROVIDER_H_DEFINED

//local includes
#include "data.h"

namespace Playlist
{
  namespace Item
  {
    class DetectParameters
    {
    public:
      virtual ~DetectParameters() {}

      virtual Parameters::Container::Ptr CreateInitialAdjustedParameters() const = 0;
      virtual bool ProcessItem(Data::Ptr item) = 0;
      virtual void ShowProgress(unsigned progress) = 0;
      virtual void ShowMessage(const String& message) = 0;
    };

    class DataProvider
    {
    public:
      typedef boost::shared_ptr<const DataProvider> Ptr;

      virtual ~DataProvider() {}

      virtual Error DetectModules(const String& path, DetectParameters& detectParams) const = 0;

      virtual Error OpenModule(const String& path, DetectParameters& detectParams) const = 0;

      static Ptr Create(Parameters::Accessor::Ptr parameters);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_DATA_PROVIDER_H_DEFINED
