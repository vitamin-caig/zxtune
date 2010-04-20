/*
Abstract:
  Playback control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
#define ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED

//local includes
#include "playback_controls_ui.h"

class PlaybackControls : public QWidget
                       , public Ui::PlaybackControls
{
  Q_OBJECT
public:
  PlaybackControls(QWidget* parent = 0);
};

#endif //ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
