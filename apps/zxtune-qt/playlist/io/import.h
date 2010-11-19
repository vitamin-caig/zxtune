/*
Abstract:
  Playlist import interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_IO_IMPORT_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_IO_IMPORT_H_DEFINED

//local includes
#include "container.h"
#include "supp/playitems_provider.h"

namespace Playlist
{
  namespace IO
  {
    //common
    Container::Ptr Open(PlayitemsProvider::Ptr provider, const class QString& filename);
    //specific
    Container::Ptr OpenAYL(PlayitemsProvider::Ptr provider, const class QString& filename);
    Container::Ptr OpenXSPF(PlayitemsProvider::Ptr provider, const class QString& filename);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_IO_IMPORT_H_DEFINED
