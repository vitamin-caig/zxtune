/*
Abstract:
  Sound settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SOUND_H_DEFINED
#define ZXTUNE_QT_UI_Sound_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class SoundSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SoundSettingsWidget(QWidget& parent);
  public:
    static SoundSettingsWidget* Create(QWidget& parent, bool playing);
  private slots:
    virtual void ChangeSoundFrequency(int idx) = 0;
  };
}

#endif //ZXTUNE_QT_UI_AYM_H_DEFINED
