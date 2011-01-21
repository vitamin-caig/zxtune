/*
Abstract:
  Volume control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_VOLUMECONTROL_H_DEFINED
#define ZXTUNE_QT_VOLUMECONTROL_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace ZXTune
{
  namespace Sound
  {
    class Backend;
  }
}

class PlaybackSupport;

class VolumeControl : public QWidget
{
  Q_OBJECT
protected:
  explicit VolumeControl(QWidget& parent);
public:
  //creator
  static VolumeControl* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void SetBackend(const ZXTune::Sound::Backend&) = 0;
  virtual void UpdateState() = 0;
private slots:
  virtual void SetLevel(int level) = 0;
};

#endif //ZXTUNE_QT_VOLUMECONTROL_H_DEFINED
