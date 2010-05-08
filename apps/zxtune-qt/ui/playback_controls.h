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

//TODO: hide internals
class PlaybackControls : public QWidget
                       , public Ui::PlaybackControls
{
  Q_OBJECT
public:
  static PlaybackControls* Create(QWidget* parent)
  {
    return new PlaybackControls(parent);
  }
private:
  PlaybackControls(QWidget* parent);
};

#endif //ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
