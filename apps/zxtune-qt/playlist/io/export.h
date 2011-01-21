/*
Abstract:
  Playlist export interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_IO_EXPORT_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_IO_EXPORT_H_DEFINED

//local includes
#include "container.h"

class QString;
namespace Playlist
{
  namespace IO
  {
    bool SaveXSPF(Container::Ptr container, const QString& filename);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_IO_EXPORT_H_DEFINED
