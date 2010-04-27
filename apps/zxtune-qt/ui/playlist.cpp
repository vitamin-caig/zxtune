/*
Abstract:
  Playlist creating implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist.h"
#include "playlist_moc.h"

Playlist::Playlist(QWidget* parent)
  : QWidget(parent)
{
  setupUi(this);
  scanStatus->hide();
}
