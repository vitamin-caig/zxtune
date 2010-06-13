/*
Abstract:
  Volume control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_VOLUMECONTROL_H_DEFINED
#define ZXTUNE_QT_VOLUMECONTROL_H_DEFINED

//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class VolumeControl : public QWidget
{
  Q_OBJECT
public:
  //creator
  static VolumeControl* Create(QWidget* parent);

public slots:
  virtual void SetBackend(const ZXTune::Sound::Backend&) = 0;
  virtual void UpdateState() = 0;
private slots:
  virtual void SetLevel(int level) = 0;
};

#endif //ZXTUNE_QT_VOLUMECONTROL_H_DEFINED
