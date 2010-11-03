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

//qt includes
#include <QtGui/QWidget>

class PlaybackControls : public QWidget
{
  Q_OBJECT
public:
  //creator
  static PlaybackControls* Create(QWidget* parent);

  virtual class QMenu* GetActionsMenu() const = 0;
signals:
  void OnPlay();
  void OnPause();
  void OnStop();
  void OnPrevious();
  void OnNext();
};

#endif //ZXTUNE_QT_PLAYBACKCONTROL_H_DEFINED
