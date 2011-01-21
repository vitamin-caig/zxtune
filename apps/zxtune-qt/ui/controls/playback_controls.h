/*
Abstract:
  Playback control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
#define ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED

//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class PlaybackControls : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackControls(QWidget& parent);
public:
  //creator
  static PlaybackControls* Create(QWidget& parent, PlaybackSupport& supp);

  virtual class QMenu* GetActionsMenu() const = 0;
signals:
  void OnPlay();
  void OnPause();
  void OnStop();
  void OnPrevious();
  void OnNext();
};

#endif //ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
