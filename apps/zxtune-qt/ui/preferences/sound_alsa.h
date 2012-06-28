/*
Abstract:
  ALSA settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SOUND_ALSA_H_DEFINED
#define ZXTUNE_QT_UI_SOUND_ALSA_H_DEFINED

//local includes
#include "../conversion/backend_settings.h"

namespace UI
{
  class AlsaSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit AlsaSettingsWidget(QWidget& parent);
  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void DeviceChanged(const QString& name) = 0;
    virtual void MixerChanged(const QString& name) = 0;
  };
}

#endif //ZXTUNE_QT_UI_SOUND_ALSA_H_DEFINED
