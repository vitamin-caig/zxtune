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
#include "playlist/supp/data_provider.h"

class QString;
namespace Playlist
{
  namespace IO
  {
    //common
    Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename);
    //specific
    Container::Ptr OpenAYL(Item::DataProvider::Ptr provider, const QString& filename);
    Container::Ptr OpenXSPF(Item::DataProvider::Ptr provider, const QString& filename);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_IO_IMPORT_H_DEFINED
