/*
Abstract:
  SDL settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SOUND_SDL_H_DEFINED
#define ZXTUNE_QT_UI_SOUND_SDL_H_DEFINED

//local includes
#include "../conversion/backend_settings.h"

namespace UI
{
  class SdlSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit SdlSettingsWidget(QWidget& parent);
  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  };
}

#endif //ZXTUNE_QT_UI_SOUND_SDL_H_DEFINED
