/*
Abstract:
  Seek control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_SEEKCONTROL_H_DEFINED
#define ZXTUNE_QT_SEEKCONTROL_H_DEFINED

//library includes
#include <core/module_player.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class SeekControls : public QWidget
{
  Q_OBJECT
protected:
  explicit SeekControls(QWidget& parent);
public:
  //creator
  static SeekControls* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void InitState(ZXTune::Module::Player::ConstPtr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
signals:
  void OnSeeking(int);
};

#endif //ZXTUNE_QT_SEEKBACKCONTROL_H_DEFINED
