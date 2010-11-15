/*
Abstract:
  Playlist container interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//common includes
#include <parameters.h>

class PlaylistIOContainer
{
public:
  typedef boost::shared_ptr<const PlaylistIOContainer> Ptr;

  virtual ~PlaylistIOContainer() {}

  virtual Parameters::Accessor::Ptr GetProperties() const = 0;
  virtual Playitem::Iterator::Ptr GetItems() const = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_IO_CONTAINER_H_DEFINED
