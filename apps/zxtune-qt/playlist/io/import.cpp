/*
Abstract:
  Playlist import implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#include "import.h"

PlaylistIOContainer::Ptr OpenPlaylist(PlayitemsProvider::Ptr provider, const QString& filename)
{
  if (PlaylistIOContainer::Ptr res = OpenAYLPlaylist(provider, filename))
  {
    return res;
  }
  else if (PlaylistIOContainer::Ptr res = OpenXSPFPlaylist(provider, filename))
  {
    return res;
  }
  return PlaylistIOContainer::Ptr();
}
