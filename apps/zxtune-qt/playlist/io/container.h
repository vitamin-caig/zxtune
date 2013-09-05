/*
Abstract:
  Playlist container interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED

//local includes
#include "playlist/supp/data.h"
//library includes
#include <parameters/accessor.h>

namespace Playlist
{
  const Parameters::NameType ATTRIBUTES_PREFIX("zxtune.app.playlist");

  const Parameters::NameType ATTRIBUTE_NAME = ATTRIBUTES_PREFIX + "name";
  const Parameters::NameType ATTRIBUTE_VERSION = ATTRIBUTES_PREFIX + "version"; 
  const Parameters::NameType ATTRIBUTE_CREATOR = ATTRIBUTES_PREFIX + "creator";
  const Parameters::NameType ATTRIBUTE_ITEMS = ATTRIBUTES_PREFIX + "items";

  namespace IO
  {
    class Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      virtual ~Container() {}

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual unsigned GetItemsCount() const = 0;
      virtual Item::Collection::Ptr GetItems() const = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
