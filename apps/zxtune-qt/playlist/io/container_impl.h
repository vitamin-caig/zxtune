/*
Abstract:
  Playlist container internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED

//local includes
#include "container.h"
//common includes
#include <parameters.h>

struct PlaylistContainerItem
{
  String Path;
  Parameters::Accessor::Ptr AdjustedParameters;
};

typedef std::vector<PlaylistContainerItem> PlaylistContainerItems;
typedef boost::shared_ptr<const PlaylistContainerItems> PlaylistContainerItemsPtr;

PlaylistIOContainer::Ptr CreatePlaylistIOContainer(PlayitemsProvider::Ptr provider,
  Parameters::Accessor::Ptr properties,
  PlaylistContainerItemsPtr items);

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED
