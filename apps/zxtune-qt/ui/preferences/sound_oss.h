/*
Abstract:
  OSS settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SOUND_OSS_H_DEFINED
#define ZXTUNE_QT_UI_SOUND_OSS_H_DEFINED

//local includes
#include "../conversion/backend_settings.h"

namespace UI
{
  class OssSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit OssSettingsWidget(QWidget& parent);
  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void DeviceSelected() = 0;
    virtual void MixerSelected() = 0;
  };
}

#endif //ZXTUNE_QT_UI_SOUND_OSS_H_DEFINED
