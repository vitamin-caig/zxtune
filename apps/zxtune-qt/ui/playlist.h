/*
Abstract:
  Playlist widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_H_DEFINED

//local includes
#include "playlist_ui.h"

class Playlist : public QWidget
               , public Ui::Playlist
{
  Q_OBJECT
public:
  Playlist(QWidget* parent = 0);
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
